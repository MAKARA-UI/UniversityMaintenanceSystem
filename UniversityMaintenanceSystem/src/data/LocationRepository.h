#pragma once

#include <QList>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>

struct CampusRoom
{
    qint64 id = 0;
    QString campus;
    QString building;
    QString room;
};

class LocationRepository
{
public:
    explicit LocationRepository(const QString &databasePath);
    ~LocationRepository();

    LocationRepository(const LocationRepository &) = delete;
    LocationRepository &operator=(const LocationRepository &) = delete;

    bool initialize(QString *error = nullptr);
    bool add(const QString &campus, const QString &building,
             const QString &room, QString *error = nullptr);
    QList<CampusRoom> all(QString *error = nullptr) const;
    QStringList buildings(const QString &campus) const;
    QStringList rooms(const QString &campus, const QString &building) const;

private:
    QSqlDatabase m_database;
    QString m_connectionName;
};
