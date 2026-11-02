#pragma once

#include <QList>
#include <QSqlDatabase>
#include <QString>

struct WorkOrder
{
    qint64 id = 0;
    qint64 faultId = 0;
    QString campus;
    QString building;
    QString room;
    QString category;
    QString priority;
    QString faultStatus;
    QString technician;
    QString status;
    QString workPerformed;
    QString partsRequested;
    QString partsUsed;
};

class WorkOrderRepository
{
public:
    explicit WorkOrderRepository(const QString &databasePath);
    ~WorkOrderRepository();

    WorkOrderRepository(const WorkOrderRepository &) = delete;
    WorkOrderRepository &operator=(const WorkOrderRepository &) = delete;

    bool initialize(QString *error = nullptr);
    bool assign(qint64 faultId, const QString &technician,
                const QString &assignedBy, QString *error = nullptr);
    QList<WorkOrder> all(const QString &technician = {},
                         QString *error = nullptr) const;
    bool update(qint64 workOrderId, const QString &status,
                const QString &workPerformed, const QString &partsUsed,
                QString *error = nullptr);
    bool requestParts(qint64 workOrderId, const QString &description,
                      QString *error = nullptr);
    bool close(qint64 workOrderId, QString *error = nullptr);

private:
    QSqlDatabase m_database;
    QString m_connectionName;
};
