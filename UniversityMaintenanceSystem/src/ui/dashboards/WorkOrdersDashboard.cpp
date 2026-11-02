#include "ui/MainWindow.h"
#include "data/WorkOrderRepository.h"
#include "domain/FaultReport.h"

#include <QTableWidget>
#include <QTableWidgetItem>
#include <QStatusBar>
#include <QStringList>
void MainWindow::refreshWorkOrders()
{
    if (!m_workOrdersTable) return;
    QString error;
    const QString technician = m_role == "Technician" ? m_username : QString();
    const QList<WorkOrder> orders = m_workOrders.all(technician, &error);
    if (!error.isEmpty()) {
        statusBar()->showMessage(error);
        return;
    }
    m_workOrdersTable->setRowCount(orders.size());
    for (int row = 0; row < orders.size(); ++row) {
        const WorkOrder &order = orders.at(row);
        const QStringList values{
            QString("WO-%1").arg(order.id, 6, 10, QChar('0')),
            QString("FR-%1").arg(order.faultId, 6, 10, QChar('0')),
            order.campus, order.building + " / " + order.room,
            order.category, order.technician, order.priority, order.status};
        for (int column = 0; column < values.size(); ++column) {
            auto *item = new QTableWidgetItem(values.at(column));
            if (column == 0) item->setData(Qt::UserRole, order.id);
            m_workOrdersTable->setItem(row, column, item);
        }
    }
}
