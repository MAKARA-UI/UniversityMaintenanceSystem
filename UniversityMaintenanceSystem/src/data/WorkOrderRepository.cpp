#include "WorkOrderRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QUuid>

WorkOrderRepository::WorkOrderRepository(const QString &databasePath)
    : m_connectionName("work-orders-" + QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    m_database = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    m_database.setDatabaseName(databasePath);
}

WorkOrderRepository::~WorkOrderRepository()
{
    const QString connectionName = m_database.connectionName();
    m_database.close();
    m_database = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionName);
}

bool WorkOrderRepository::initialize(QString *error)
{
    if (!m_database.open()) {
        if (error) *error = "Could not open the work-order database: " + m_database.lastError().text();
        return false;
    }
    QSqlQuery query(m_database);
    if (!query.exec("PRAGMA foreign_keys = ON")) {
        if (error) *error = "Could not enable work-order links: " + query.lastError().text();
        return false;
    }
    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS work_orders ("
            "work_order_id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "fault_id INTEGER NOT NULL UNIQUE REFERENCES fault_reports(fault_id),"
            "technician_username TEXT NOT NULL, assigned_by TEXT NOT NULL,"
            "status TEXT NOT NULL DEFAULT 'Assigned' CHECK (status IN "
            "('Assigned','Accepted','In Progress','Awaiting Parts','Completed - Review','Closed')),"
            "work_performed TEXT NOT NULL DEFAULT '', parts_requested TEXT NOT NULL DEFAULT '',"
            "parts_used TEXT NOT NULL DEFAULT '', assigned_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP)")) {
        if (error) *error = "Could not prepare work orders: " + query.lastError().text();
        return false;
    }
    return true;
}

bool WorkOrderRepository::assign(qint64 faultId, const QString &technician,
                                 const QString &assignedBy, QString *error)
{
    QSqlQuery technicianQuery(m_database);
    technicianQuery.prepare("SELECT COUNT(*) FROM accounts WHERE username=:name "
                            "AND role='Technician' AND is_active=1");
    technicianQuery.bindValue(":name", technician);
    if (!technicianQuery.exec() || !technicianQuery.next() ||
        technicianQuery.value(0).toInt() == 0) {
        if (error) *error = "Choose an active account with the Technician role.";
        return false;
    }
    technicianQuery.finish();
    if (!m_database.transaction()) {
        if (error) *error = "Could not start a work-order assignment.";
        return false;
    }
    QSqlQuery query(m_database);
    query.prepare("INSERT INTO work_orders (fault_id, technician_username, assigned_by) "
                  "VALUES (:fault, :technician, :assigned_by)");
    query.bindValue(":fault", faultId);
    query.bindValue(":technician", technician);
    query.bindValue(":assigned_by", assignedBy);
    if (!query.exec()) {
        if (error) {
            *error = query.lastError().nativeErrorCode() == "19"
                         ? "This fault already has a work order."
                         : "Could not assign the fault: " + query.lastError().text();
        }
        m_database.rollback();
        return false;
    }
    QSqlQuery updateFault(m_database);
    updateFault.prepare("UPDATE fault_reports SET status = 'Assigned' WHERE fault_id = :fault");
    updateFault.bindValue(":fault", faultId);
    if (!updateFault.exec()) {
        if (error) *error = "Work order was created, but fault status could not be updated: "
                             + updateFault.lastError().text();
        m_database.rollback();
        return false;
    }
    if (!m_database.commit()) {
        if (error) *error = "Could not complete work-order assignment.";
        m_database.rollback();
        return false;
    }
    return true;
}

QList<WorkOrder> WorkOrderRepository::all(const QString &technician, QString *error) const
{
    QList<WorkOrder> results;
    QSqlQuery query(m_database);
    QString sql =
        "SELECT w.work_order_id,w.fault_id,f.campus,f.building,f.room,f.category,"
        "f.priority,f.status,w.technician_username,w.status,w.work_performed,"
        "w.parts_requested,w.parts_used FROM work_orders w "
        "JOIN fault_reports f ON f.fault_id=w.fault_id";
    if (!technician.isEmpty()) sql += " WHERE w.technician_username=:technician";
    sql += " ORDER BY w.work_order_id DESC";
    query.prepare(sql);
    if (!technician.isEmpty()) query.bindValue(":technician", technician);
    if (!query.exec()) {
        if (error) *error = "Could not load work orders: " + query.lastError().text();
        return results;
    }
    while (query.next()) {
        WorkOrder order;
        order.id = query.value(0).toLongLong();
        order.faultId = query.value(1).toLongLong();
        order.campus = query.value(2).toString();
        order.building = query.value(3).toString();
        order.room = query.value(4).toString();
        order.category = query.value(5).toString();
        order.priority = query.value(6).toString();
        order.faultStatus = query.value(7).toString();
        order.technician = query.value(8).toString();
        order.status = query.value(9).toString();
        order.workPerformed = query.value(10).toString();
        order.partsRequested = query.value(11).toString();
        order.partsUsed = query.value(12).toString();
        results.append(order);
    }
    return results;
}

bool WorkOrderRepository::update(qint64 workOrderId, const QString &status,
                                 const QString &workPerformed, const QString &partsUsed,
                                 QString *error)
{
    const QStringList allowed{"Accepted", "In Progress", "Awaiting Parts", "Completed - Review"};
    if (!allowed.contains(status)) {
        if (error) *error = "Choose a valid repair progress status.";
        return false;
    }
    if (status == "Completed - Review" && workPerformed.trimmed().isEmpty()) {
        if (error) *error = "Record the work performed before marking the job complete.";
        return false;
    }
    if (!m_database.transaction()) {
        if (error) *error = "Could not start a work-order update.";
        return false;
    }
    QSqlQuery order(m_database);
    order.prepare("UPDATE work_orders SET status=:status, work_performed=:work, "
                  "parts_used=:parts, updated_at=CURRENT_TIMESTAMP WHERE work_order_id=:id");
    order.bindValue(":status", status);
    order.bindValue(":work", workPerformed.trimmed());
    order.bindValue(":parts", partsUsed.trimmed());
    order.bindValue(":id", workOrderId);
    if (!order.exec()) {
        if (error) *error = "Could not save repair progress: " + order.lastError().text();
        m_database.rollback();
        return false;
    }

    QString faultStatus = "Assigned";
    if (status == "In Progress") faultStatus = "In Progress";
    else if (status == "Awaiting Parts") faultStatus = "Awaiting Parts";
    else if (status == "Completed - Review") faultStatus = "Resolved";
    QSqlQuery fault(m_database);
    fault.prepare("UPDATE fault_reports SET status=:status WHERE fault_id="
                  "(SELECT fault_id FROM work_orders WHERE work_order_id=:id)");
    fault.bindValue(":status", faultStatus);
    fault.bindValue(":id", workOrderId);
    if (!fault.exec() || !m_database.commit()) {
        if (error) *error = "Could not update the fault status: " + fault.lastError().text();
        m_database.rollback();
        return false;
    }
    return true;
}

bool WorkOrderRepository::requestParts(qint64 workOrderId, const QString &description,
                                       QString *error)
{
    if (description.trimmed().isEmpty()) {
        if (error) *error = "Describe the spare parts needed.";
        return false;
    }
    QSqlQuery query(m_database);
    query.prepare("UPDATE work_orders SET parts_requested=:request, status='Awaiting Parts', "
                  "updated_at=CURRENT_TIMESTAMP WHERE work_order_id=:id");
    query.bindValue(":request", description.trimmed());
    query.bindValue(":id", workOrderId);
    if (!query.exec() || query.numRowsAffected() != 1) {
        if (error) *error = "Could not save the parts request: " + query.lastError().text();
        return false;
    }
    QSqlQuery fault(m_database);
    fault.prepare("UPDATE fault_reports SET status='Awaiting Parts' WHERE fault_id="
                  "(SELECT fault_id FROM work_orders WHERE work_order_id=:id)");
    fault.bindValue(":id", workOrderId);
    if (!fault.exec()) {
        if (error) *error = "Parts request saved, but fault status could not update: "
                             + fault.lastError().text();
        return false;
    }
    return true;
}

bool WorkOrderRepository::close(qint64 workOrderId, QString *error)
{
    QSqlQuery query(m_database);
    query.prepare("UPDATE work_orders SET status='Closed', updated_at=CURRENT_TIMESTAMP "
                  "WHERE work_order_id=:id AND status='Completed - Review'");
    query.bindValue(":id", workOrderId);
    if (!query.exec() || query.numRowsAffected() != 1) {
        if (error) *error = "Only work orders marked completed can be closed for review.";
        return false;
    }
    QSqlQuery fault(m_database);
    fault.prepare("UPDATE fault_reports SET status='Closed' WHERE fault_id="
                  "(SELECT fault_id FROM work_orders WHERE work_order_id=:id)");
    fault.bindValue(":id", workOrderId);
    if (!fault.exec()) {
        if (error) *error = "The work order closed, but the fault status could not update.";
        return false;
    }
    return true;
}
