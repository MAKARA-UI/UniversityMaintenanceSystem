#include "ui/MainWindow.h"
#include "ui/UiHelpers.h"

#include "data/AccountRepository.h"
#include "data/FaultRepository.h"
#include "data/WorkOrderRepository.h"
#include "domain/FaultReport.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDialog>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QStatusBar>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>

using namespace UiHelpers;

QWidget *MainWindow::createManagerWorkOrdersPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(12);
    layout->addWidget(makeLabel(
        "Assign reported faults to technicians, monitor repair progress, and close completed work.",
        "infoText", page));
    m_workOrdersTable = new QTableWidget(0, 8, page);
    m_workOrdersTable->setHorizontalHeaderLabels(
        {"Work order", "Fault", "Campus", "Building / Room", "Category", "Technician", "Priority", "Status"});
    m_workOrdersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_workOrdersTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_workOrdersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_workOrdersTable->setAlternatingRowColors(true);
    m_workOrdersTable->verticalHeader()->setVisible(false);
    m_workOrdersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(m_workOrdersTable, 1);
    auto *closeButton = new QPushButton("Close selected completed work order", page);
    closeButton->setObjectName("primaryButton");
    closeButton->setMinimumHeight(44);
    layout->addWidget(closeButton, 0, Qt::AlignRight);
    connect(closeButton, &QPushButton::clicked, this, &MainWindow::closeSelectedWorkOrder);
    return page;
}

QWidget *MainWindow::createMaintenanceReportsPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->addWidget(makeLabel("Current maintenance status summary", "sectionTitle", page));
    m_maintenanceReportsTable = new QTableWidget(0, 2, page);
    m_maintenanceReportsTable->setHorizontalHeaderLabels({"Measure", "Count"});
    m_maintenanceReportsTable->verticalHeader()->setVisible(false);
    m_maintenanceReportsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_maintenanceReportsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_maintenanceReportsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    layout->addWidget(m_maintenanceReportsTable, 1);
    refreshMaintenanceReports();
    return page;
}
void MainWindow::refreshMaintenanceReports()
{
    if (!m_maintenanceReportsTable) return;
    QString error;
    const auto faults = m_repository.latest(5000, &error);
    QHash<QString, int> byStatus;
    int urgent = 0;
    for (const FaultReport &fault : faults) {
        ++byStatus[fault.status];
        if (fault.priority == "Critical" || fault.priority == "High") ++urgent;
    }
    const auto orders = m_workOrders.all({}, &error);
    const QList<QPair<QString, int>> metrics{
        {"Total reported faults", faults.size()},
        {"Submitted / awaiting review", byStatus.value("Submitted")},
        {"Assigned", byStatus.value("Assigned")},
        {"In progress", byStatus.value("In Progress")},
        {"Awaiting parts", byStatus.value("Awaiting Parts")},
        {"Resolved", byStatus.value("Resolved")},
        {"Closed", byStatus.value("Closed")},
        {"Critical or high priority", urgent},
        {"Work orders created", orders.size()}
    };
    m_maintenanceReportsTable->setRowCount(metrics.size());
    for (int row = 0; row < metrics.size(); ++row) {
        m_maintenanceReportsTable->setItem(row, 0, new QTableWidgetItem(metrics.at(row).first));
        m_maintenanceReportsTable->setItem(row, 1,
            new QTableWidgetItem(QString::number(metrics.at(row).second)));
    }
}
void MainWindow::assignSelectedFault()
{
    const int row = m_reportsTable ? m_reportsTable->currentRow() : -1;
    if (row < 0 || !m_reportsTable->item(row, 0)) {
        showPlainMessage(this, "Select a fault", "Choose a fault from the queue first.");
        return;
    }
    QString error;
    const QList<UserAccount> accounts = m_accounts.allAccounts(&error);
    QStringList technicians;
    for (const UserAccount &account : accounts) {
        if (account.role == "Technician" && account.active) technicians.append(account.username);
    }
    if (technicians.isEmpty()) {
        showPlainMessage(this, "No technicians yet",
                         "Register an account with the Technician role before assigning work.");
        return;
    }
    QDialog assignDialog(this);
    assignDialog.setWindowTitle("Assign technician");
    assignDialog.setModal(true);
    assignDialog.setMinimumWidth(400);
    auto *assignLayout = new QVBoxLayout(&assignDialog);
    assignLayout->setContentsMargins(24, 22, 24, 20);
    assignLayout->setSpacing(16);
    assignLayout->addWidget(new QLabel("Choose an active technician for this fault.", &assignDialog));
    auto *technicianCombo = new QComboBox(&assignDialog);
    technicianCombo->addItems(technicians);
    assignLayout->addWidget(technicianCombo);
    auto *assignActions = new QHBoxLayout;
    assignActions->addStretch(1);
    auto *cancel = new QPushButton("Cancel", &assignDialog);
    auto *assignButton = new QPushButton("Assign", &assignDialog);
    assignButton->setObjectName("primaryButton");
    assignActions->addWidget(cancel);
    assignActions->addWidget(assignButton);
    assignLayout->addLayout(assignActions);
    connect(cancel, &QPushButton::clicked, &assignDialog, &QDialog::reject);
    connect(assignButton, &QPushButton::clicked, &assignDialog, &QDialog::accept);
    if (assignDialog.exec() != QDialog::Accepted) return;
    const QString technician = technicianCombo->currentText();

    const qint64 faultId = m_reportsTable->item(row, 0)->data(Qt::UserRole).toLongLong();
    if (!m_workOrders.assign(faultId, technician, m_username, &error)) {
        showPlainMessage(this, "Could not create work order", error);
        return;
    }
    statusBar()->showMessage("Work order assigned to " + technician, 5000);
    refreshReports();
    refreshHomeSummary();
    refreshWorkOrders();
    refreshMaintenanceReports();
    showSuccess("Work order assigned", "The work order was created and assigned to " + technician + ".");
}
void MainWindow::closeSelectedWorkOrder()
{
    const int row = m_workOrdersTable ? m_workOrdersTable->currentRow() : -1;
    if (row < 0 || !m_workOrdersTable->item(row, 0)) {
        showPlainMessage(this, "Select a work order", "Choose a completed work order first.");
        return;
    }
    const qint64 id = m_workOrdersTable->item(row, 0)->data(Qt::UserRole).toLongLong();
    QString error;
    if (!m_workOrders.close(id, &error)) {
        showPlainMessage(this, "Could not close work order", error);
        return;
    }
    refreshWorkOrders();
    refreshHomeSummary();
    refreshMaintenanceReports();
    showSuccess("Work order closed", "The completed work order has been closed and the fault status refreshed.");
}
