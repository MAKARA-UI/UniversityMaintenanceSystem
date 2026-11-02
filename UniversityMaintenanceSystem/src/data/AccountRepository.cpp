#include "AccountRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUuid>

#include <openssl/evp.h>
#include <openssl/crypto.h>
#include <openssl/rand.h>

namespace {
constexpr int SaltSize = 16;
constexpr int HashSize = 32;
constexpr int Iterations = 310000;

bool derivePasswordHash(const QString &password, const QByteArray &salt,
                        QByteArray *hash)
{
    const QByteArray passwordBytes = password.toUtf8();
    hash->resize(HashSize);
    return PKCS5_PBKDF2_HMAC(passwordBytes.constData(), passwordBytes.size(),
                             reinterpret_cast<const unsigned char *>(salt.constData()),
                             salt.size(), Iterations, EVP_sha256(), HashSize,
                             reinterpret_cast<unsigned char *>(hash->data())) == 1;
}
}

AccountRepository::AccountRepository(const QString &databasePath)
    : m_connectionName("accounts-" + QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    m_database = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    m_database.setDatabaseName(databasePath);
}

AccountRepository::~AccountRepository()
{
    const QString connectionName = m_database.connectionName();
    m_database.close();
    m_database = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionName);
}

bool AccountRepository::initialize(QString *error)
{
    if (!m_database.open()) {
        if (error) *error = "Could not open the account database: " + m_database.lastError().text();
        return false;
    }

    QSqlQuery query(m_database);
    if (!query.exec("CREATE TABLE IF NOT EXISTS accounts ("
                    "account_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "username TEXT NOT NULL COLLATE NOCASE UNIQUE,"
                    "password_salt TEXT NOT NULL, password_hash TEXT NOT NULL,"
                    "role TEXT NOT NULL DEFAULT 'Reporter',"
                    "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP)")) {
        if (error) *error = "Could not prepare the accounts table: " + query.lastError().text();
        return false;
    }

    bool hasRoleColumn = false;
    if (!query.exec("PRAGMA table_info(accounts)")) {
        if (error) *error = "Could not inspect the accounts table: " + query.lastError().text();
        return false;
    }
    const int nameColumn = query.record().indexOf("name");
    while (query.next()) {
        if (query.value(nameColumn).toString() == "role") {
            hasRoleColumn = true;
            break;
        }
    }
    query.finish();
    if (!hasRoleColumn) {
        QSqlQuery migration(m_database);
        if (!migration.exec(
                "ALTER TABLE accounts ADD COLUMN role TEXT NOT NULL DEFAULT 'Reporter'")) {
            if (error) *error = "Could not add account roles: " + migration.lastError().text();
            return false;
        }
        if (!migration.exec(
                "UPDATE accounts SET role = 'Administrator' "
                "WHERE account_id = (SELECT MIN(account_id) FROM accounts) "
                "AND NOT EXISTS (SELECT 1 FROM accounts WHERE role = 'Administrator')")) {
            if (error) *error = "Could not assign the initial administrator role: "
                                 + migration.lastError().text();
            return false;
        }
    }
    bool hasActiveColumn = false;
    QSqlQuery columns(m_database);
    if (!columns.exec("PRAGMA table_info(accounts)")) {
        if (error) *error = "Could not inspect account status: " + columns.lastError().text();
        return false;
    }
    const int columnName = columns.record().indexOf("name");
    while (columns.next()) {
        if (columns.value(columnName).toString() == "is_active") {
            hasActiveColumn = true;
            break;
        }
    }
    columns.finish();
    if (!hasActiveColumn && !columns.exec(
            "ALTER TABLE accounts ADD COLUMN is_active INTEGER NOT NULL DEFAULT 1")) {
        if (error) *error = "Could not add account activation status: " + columns.lastError().text();
        return false;
    }
    return true;
}

bool AccountRepository::createAccount(const QString &username, const QString &password,
                                      const QString &role, QString *error)
{
    if (!availableRoles().contains(role)) {
        if (error) *error = "Select one of the supported account roles.";
        return false;
    }
    QByteArray salt(SaltSize, '\0');
    if (RAND_bytes(reinterpret_cast<unsigned char *>(salt.data()), salt.size()) != 1) {
        if (error) *error = "Secure password setup failed. Please try again.";
        return false;
    }
    QByteArray hash;
    if (!derivePasswordHash(password, salt, &hash)) {
        if (error) *error = "Secure password setup failed. Please try again.";
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare("INSERT INTO accounts (username, password_salt, password_hash, role) "
                  "VALUES (:username, :salt, :hash, :role)");
    query.bindValue(":username", username.trimmed());
    query.bindValue(":salt", QString::fromLatin1(salt.toHex()));
    query.bindValue(":hash", QString::fromLatin1(hash.toHex()));
    query.bindValue(":role", role);
    if (!query.exec()) {
        if (error) {
            *error = query.lastError().nativeErrorCode() == "19"
                         ? "That username is already registered. Choose another one."
                         : "Could not create the account: " + query.lastError().text();
        }
        return false;
    }
    return true;
}

bool AccountRepository::authenticate(const QString &username, const QString &password,
                                     QString *role, QString *error) const
{
    QSqlQuery query(m_database);
    query.prepare("SELECT password_salt, password_hash, role, is_active FROM accounts WHERE username = :username");
    query.bindValue(":username", username.trimmed());
    if (!query.exec()) {
        if (error) *error = "Could not check the account: " + query.lastError().text();
        return false;
    }
    if (!query.next()) {
        if (error) *error = "Username or password is incorrect.";
        return false;
    }
    if (query.value(3).toInt() == 0) {
        if (error) *error = "This account is inactive. Contact an administrator.";
        return false;
    }

    const QByteArray salt = QByteArray::fromHex(query.value(0).toString().toLatin1());
    const QByteArray storedHash = QByteArray::fromHex(query.value(1).toString().toLatin1());
    QByteArray calculatedHash;
    if (salt.size() != SaltSize || storedHash.size() != HashSize ||
        !derivePasswordHash(password, salt, &calculatedHash) ||
        CRYPTO_memcmp(storedHash.constData(), calculatedHash.constData(), HashSize) != 0) {
        if (error) *error = "Username or password is incorrect.";
        return false;
    }
    if (role) *role = query.value(2).toString();
    return true;
}

QList<UserAccount> AccountRepository::allAccounts(QString *error) const
{
    QList<UserAccount> accounts;
    QSqlQuery query(m_database);
    if (!query.exec("SELECT account_id, username, role, is_active FROM accounts ORDER BY username COLLATE NOCASE")) {
        if (error) *error = "Could not load user accounts: " + query.lastError().text();
        return accounts;
    }
    while (query.next()) {
        accounts.append({query.value(0).toLongLong(), query.value(1).toString(),
                         query.value(2).toString(), query.value(3).toInt() != 0});
    }
    return accounts;
}

QStringList AccountRepository::availableRoles()
{
    return {"Administrator", "Facilities manager", "Technician", "Reporter"};
}

bool AccountRepository::updateRole(qint64 accountId, const QString &role, QString *error)
{
    if (!availableRoles().contains(role)) {
        if (error) *error = "Select one of the supported account roles.";
        return false;
    }
    QSqlQuery query(m_database);
    query.prepare("UPDATE accounts SET role = :role WHERE account_id = :id");
    query.bindValue(":role", role);
    query.bindValue(":id", accountId);
    if (!query.exec()) {
        if (error) *error = "Could not update the account role: " + query.lastError().text();
        return false;
    }
    return query.numRowsAffected() == 1;
}

bool AccountRepository::setActive(qint64 accountId, bool active, QString *error)
{
    QSqlQuery query(m_database);
    query.prepare("UPDATE accounts SET is_active=:active WHERE account_id=:id");
    query.bindValue(":active", active ? 1 : 0);
    query.bindValue(":id", accountId);
    if (!query.exec()) {
        if (error) *error = "Could not change account activation: " + query.lastError().text();
        return false;
    }
    return query.numRowsAffected() == 1;
}
