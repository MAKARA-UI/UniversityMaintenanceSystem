#include "LocationRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

LocationRepository::LocationRepository(const QString &databasePath)
    : m_connectionName("locations-" + QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    m_database = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    m_database.setDatabaseName(databasePath);
}

LocationRepository::~LocationRepository()
{
    const QString connectionName = m_database.connectionName();
    m_database.close();
    m_database = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionName);
}

bool LocationRepository::initialize(QString *error)
{
    if (!m_database.open()) {
        if (error) *error = "Could not open the campus-location database: " + m_database.lastError().text();
        return false;
    }
    QSqlQuery query(m_database);
    if (!query.exec("CREATE TABLE IF NOT EXISTS campus_rooms ("
                    "room_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "campus TEXT NOT NULL CHECK (campus IN ('Main Campus','MP Campus')),"
                    "building TEXT NOT NULL CHECK (length(trim(building)) BETWEEN 1 AND 100),"
                    "room TEXT NOT NULL CHECK (length(trim(room)) BETWEEN 1 AND 50),"
                    "UNIQUE(campus, building, room))")) {
        if (error) *error = "Could not prepare campus locations: " + query.lastError().text();
        return false;
    }
    return true;
}

bool LocationRepository::add(const QString &campus, const QString &building,
                             const QString &room, QString *error)
{
    if (campus != "Main Campus" && campus != "MP Campus") {
        if (error) *error = "Choose Main Campus or MP Campus.";
        return false;
    }
    QSqlQuery query(m_database);
    query.prepare("INSERT INTO campus_rooms (campus, building, room) "
                  "VALUES (:campus, :building, :room)");
    query.bindValue(":campus", campus);
    query.bindValue(":building", building.trimmed());
    query.bindValue(":room", room.trimmed());
    if (!query.exec()) {
        if (error) {
            *error = query.lastError().nativeErrorCode() == "19"
                         ? "That building and room are already registered for this campus."
                         : "Could not register the location: " + query.lastError().text();
        }
        return false;
    }
    return true;
}

QList<CampusRoom> LocationRepository::all(QString *error) const
{
    QList<CampusRoom> result;
    QSqlQuery query(m_database);
    if (!query.exec("SELECT room_id,campus,building,room FROM campus_rooms "
                    "ORDER BY campus,building,room")) {
        if (error) *error = "Could not load campus locations: " + query.lastError().text();
        return result;
    }
    while (query.next()) {
        result.append({query.value(0).toLongLong(), query.value(1).toString(),
                       query.value(2).toString(), query.value(3).toString()});
    }
    return result;
}

QStringList LocationRepository::buildings(const QString &campus) const
{
    QStringList result;
    QSqlQuery query(m_database);
    query.prepare("SELECT DISTINCT building FROM campus_rooms WHERE campus=:campus ORDER BY building");
    query.bindValue(":campus", campus);
    if (query.exec()) while (query.next()) result.append(query.value(0).toString());
    return result;
}

QStringList LocationRepository::rooms(const QString &campus, const QString &building) const
{
    QStringList result;
    QSqlQuery query(m_database);
    query.prepare("SELECT room FROM campus_rooms WHERE campus=:campus AND building=:building ORDER BY room");
    query.bindValue(":campus", campus);
    query.bindValue(":building", building);
    if (query.exec()) while (query.next()) result.append(query.value(0).toString());
    return result;
}
