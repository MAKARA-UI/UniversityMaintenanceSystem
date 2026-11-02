#pragma once

#include "domain/FaultReport.h"

#include <QList>
#include <QSqlDatabase>
#include <QString>

class FaultRepository
{
public:
    explicit FaultRepository(const QString &databasePath);
    ~FaultRepository();

    FaultRepository(const FaultRepository &) = delete;
    FaultRepository &operator=(const FaultRepository &) = delete;

    bool initialize(QString *error = nullptr);
    bool add(FaultReport &report, QString *error = nullptr);
    bool setPriority(qint64 reportId, const QString &priority,
                     QString *error = nullptr);
    QList<FaultReport> latest(int limit = 200, QString *error = nullptr,
                              const QString &reportedBy = {}) const;

private:
    QSqlDatabase m_database;
    QString m_connectionName;
};
