#pragma once

#include <QSqlDatabase>
#include <QList>
#include <QString>
#include <QStringList>

struct UserAccount
{
    qint64 id = 0;
    QString username;
    QString role;
    bool active = true;
};

class AccountRepository
{
public:
    explicit AccountRepository(const QString &databasePath);
    ~AccountRepository();

    AccountRepository(const AccountRepository &) = delete;
    AccountRepository &operator=(const AccountRepository &) = delete;

    bool initialize(QString *error = nullptr);
    bool createAccount(const QString &username, const QString &password,
                       const QString &role, QString *error = nullptr);
    bool authenticate(const QString &username, const QString &password,
                      QString *role = nullptr, QString *error = nullptr) const;
    QList<UserAccount> allAccounts(QString *error = nullptr) const;
    bool updateRole(qint64 accountId, const QString &role, QString *error = nullptr);
    bool setActive(qint64 accountId, bool active, QString *error = nullptr);
    static QStringList availableRoles();

private:
    QSqlDatabase m_database;
    QString m_connectionName;
};
