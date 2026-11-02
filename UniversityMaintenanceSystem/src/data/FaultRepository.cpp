#include "FaultRepository.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStringList>
#include <QUuid>
#include <QtGlobal>

FaultRepository::FaultRepository(const QString &databasePath)
    : m_connectionName(
          "maintenance-" + QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    m_database = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    m_database.setDatabaseName(databasePath);
}

FaultRepository::~FaultRepository()
{
    const QString connectionName = m_database.connectionName();
    m_database.close();
    m_database = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionName);
}

bool FaultRepository::initialize(QString *error)
{
    if (!m_database.open()) {
        if (error) {
            *error = "Could not open the SQLite database: "
                     + m_database.lastError().text();
        }
        return false;
    }

    QSqlQuery foreignKeys(m_database);
    if (!foreignKeys.exec("PRAGMA foreign_keys = ON")) {
        if (error) {
            *error = "Could not enable SQLite foreign keys: "
                     + foreignKeys.lastError().text();
        }
        return false;
    }

    QFile schemaFile(QCoreApplication::applicationDirPath()
                     + "/database/schema.sql");
    if (!schemaFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error) {
            *error = "Could not read database/schema.sql beside the application.";
        }
        return false;
    }

    const QString schema = QString::fromUtf8(schemaFile.readAll());
    QSqlQuery createSchema(m_database);
    const QStringList statements = schema.split(';', Qt::SkipEmptyParts);
    for (const QString &statement : statements) {
        const QString sql = statement.trimmed();
        if (sql.isEmpty()) {
            continue;
        }
        if (!createSchema.exec(sql)) {
            if (error) {
                *error = "Could not initialize the database schema: "
                         + createSchema.lastError().text();
            }
            return false;
        }
    }

    bool hasReporterColumn = false;
    if (!createSchema.exec("PRAGMA table_info(fault_reports)")) {
        if (error) *error = "Could not inspect the fault-report table: "
                             + createSchema.lastError().text();
        return false;
    }
    const int nameColumn = createSchema.record().indexOf("name");
    while (createSchema.next()) {
        if (createSchema.value(nameColumn).toString() == "reported_by") {
            hasReporterColumn = true;
            break;
        }
    }
    createSchema.finish();
    if (!hasReporterColumn && !createSchema.exec(
            "ALTER TABLE fault_reports ADD COLUMN reported_by TEXT NOT NULL DEFAULT 'legacy'")) {
        if (error) *error = "Could not add the fault reporter field: "
                             + createSchema.lastError().text();
        return false;
    }

    bool hasCampusColumn = false;
    if (!createSchema.exec("PRAGMA table_info(fault_reports)")) {
        if (error) *error = "Could not inspect the campus field: " + createSchema.lastError().text();
        return false;
    }
    const int campusNameColumn = createSchema.record().indexOf("name");
    while (createSchema.next()) {
        if (createSchema.value(campusNameColumn).toString() == "campus") {
            hasCampusColumn = true;
            break;
        }
    }
    createSchema.finish();
    if (!hasCampusColumn && !createSchema.exec(
            "ALTER TABLE fault_reports ADD COLUMN campus TEXT NOT NULL DEFAULT 'Main Campus'")) {
        if (error) *error = "Could not add the campus field: " + createSchema.lastError().text();
        return false;
    }

    return true;
}

bool FaultRepository::add(FaultReport &report, QString *error)
{
    report.reportedAt = QDateTime::currentDateTime().toString(Qt::ISODate);

    QSqlQuery query(m_database);
    query.prepare(
        "INSERT INTO fault_reports "
        "(campus, building, room, category, description, urgency, priority, status, reported_at, reported_by) "
        "VALUES (:campus, :building, :room, :category, :description, :urgency, :priority, "
        ":status, :reported_at, :reported_by)");
    query.bindValue(":campus", report.campus);
    query.bindValue(":building", report.building.trimmed());
    query.bindValue(":room", report.room.trimmed());
    query.bindValue(":category", report.category);
    query.bindValue(":description", report.description.trimmed());
    query.bindValue(":urgency", report.urgency);
    query.bindValue(":priority", report.priority);
    query.bindValue(":status", report.status);
    query.bindValue(":reported_at", report.reportedAt);
    query.bindValue(":reported_by", report.reportedBy);

    if (!query.exec()) {
        if (error) {
            *error = "Could not save the fault report: "
                     + query.lastError().text();
        }
        return false;
    }

    report.id = query.lastInsertId().toLongLong();
    return true;
}

QList<FaultReport> FaultRepository::latest(int limit, QString *error,
                                           const QString &reportedBy) const
{
    QList<FaultReport> reports;
    QSqlQuery query(m_database);
    QString sql =
        "SELECT fault_id, campus, building, room, category, description, urgency, "
        "priority, status, reported_at, reported_by "
        "FROM fault_reports";
    if (!reportedBy.isEmpty()) {
        sql += " WHERE reported_by = :reported_by";
    }
    sql += " ORDER BY fault_id DESC LIMIT :limit";
    query.prepare(sql);
    query.bindValue(":limit", qMax(1, limit));
    if (!reportedBy.isEmpty()) {
        query.bindValue(":reported_by", reportedBy);
    }

    if (!query.exec()) {
        if (error) {
            *error = "Could not load fault reports: "
                     + query.lastError().text();
        }
        return reports;
    }

    while (query.next()) {
        FaultReport report;
        report.id = query.value(0).toLongLong();
        report.campus = query.value(1).toString();
        report.building = query.value(2).toString();
        report.room = query.value(3).toString();
        report.category = query.value(4).toString();
        report.description = query.value(5).toString();
        report.urgency = query.value(6).toString();
        report.priority = query.value(7).toString();
        report.status = query.value(8).toString();
        report.reportedAt = query.value(9).toString();
        report.reportedBy = query.value(10).toString();
        reports.append(report);
    }

    return reports;
}

bool FaultRepository::setPriority(qint64 reportId, const QString &priority,
                                  QString *error)
{
    if (priority != "Critical" && priority != "High" &&
        priority != "Medium" && priority != "Low") {
        if (error) *error = "Choose a valid fault priority.";
        return false;
    }
    QSqlQuery query(m_database);
    query.prepare("UPDATE fault_reports SET priority = :priority WHERE fault_id = :id");
    query.bindValue(":priority", priority);
    query.bindValue(":id", reportId);
    if (!query.exec() || query.numRowsAffected() != 1) {
        if (error) *error = "Could not update fault priority: " + query.lastError().text();
        return false;
    }
    return true;
}
