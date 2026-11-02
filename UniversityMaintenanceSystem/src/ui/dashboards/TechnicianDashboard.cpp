#include "ui/MainWindow.h"
#include "ui/UiHelpers.h"

#include "data/WorkOrderRepository.h"
#include "domain/FaultReport.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStatusBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>

using namespace UiHelpers;

QWidget *MainWindow::createTechnicianWorkOrdersPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(12);
    layout->addWidget(makeLabel(
        "Review assigned jobs, record repair progress, and track parts requests.",
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

    layout->addWidget(makeLabel("Update selected work order", "sectionTitle", page));
    auto *progressBox = new QGroupBox(page);
    auto *form = new QGridLayout(progressBox);
    m_workStatusCombo = new QComboBox(progressBox);
    m_workStatusCombo->addItems({"Accepted", "In Progress", "Awaiting Parts", "Completed - Review"});
    m_workPerformedEdit = new QPlainTextEdit(progressBox);
    m_workPerformedEdit->setPlaceholderText("Record work performed and repair notes.");
    m_workPerformedEdit->setMaximumHeight(100);
    m_partsUsedEdit = new QLineEdit(progressBox);
    m_partsUsedEdit->setPlaceholderText("Parts used, including quantities (if any)");
    m_partsRequestEdit = new QLineEdit(progressBox);
    m_partsRequestEdit->setPlaceholderText("Describe spare parts needed");
    form->addWidget(new QLabel("Progress", progressBox), 0, 0);
    form->addWidget(m_workStatusCombo, 0, 1);
    form->addWidget(new QLabel("Work performed", progressBox), 1, 0);
    form->addWidget(m_workPerformedEdit, 1, 1, 1, 3);
    form->addWidget(new QLabel("Parts used", progressBox), 2, 0);
    form->addWidget(m_partsUsedEdit, 2, 1, 1, 3);
    form->addWidget(new QLabel("Request parts", progressBox), 3, 0);
    form->addWidget(m_partsRequestEdit, 3, 1, 1, 2);
    auto *requestButton = new QPushButton("Send request", progressBox);
    requestButton->setObjectName("secondaryButton");
    form->addWidget(requestButton, 3, 3);
    auto *saveButton = new QPushButton("Save progress", progressBox);
    saveButton->setObjectName("primaryButton");
    form->addWidget(saveButton, 4, 3);
    layout->addWidget(progressBox);
    connect(saveButton, &QPushButton::clicked, this, &MainWindow::updateSelectedWorkOrder);
    connect(requestButton, &QPushButton::clicked, this, &MainWindow::requestPartsForSelectedWorkOrder);
    connect(m_workOrdersTable, &QTableWidget::currentCellChanged, this,
            [this](int row, int, int, int) {
        if (row < 0 || !m_workOrdersTable->item(row, 0)) return;
        const auto orders = m_workOrders.all(m_username);
        const qint64 selectedId = m_workOrdersTable->item(row, 0)->data(Qt::UserRole).toLongLong();
        for (const WorkOrder &order : orders) {
            if (order.id == selectedId) {
                m_workStatusCombo->setCurrentText(order.status == "Assigned" ? "Accepted" : order.status);
                m_workPerformedEdit->setPlainText(order.workPerformed);
                m_partsUsedEdit->setText(order.partsUsed);
                m_partsRequestEdit->setText(order.partsRequested);
                break;
            }
        }
    });
    return page;
}

QWidget *MainWindow::createPartsPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->addWidget(makeLabel("Parts requests and parts used are recorded on each assigned work order.", "infoText", page));
    m_partsTable = new QTableWidget(0, 4, page);
    m_partsTable->setHorizontalHeaderLabels({"Work order", "Location", "Status", "Parts requested / used"});
    m_partsTable->verticalHeader()->setVisible(false);
    m_partsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_partsTable->setAlternatingRowColors(true);
    m_partsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(m_partsTable, 1);
    return page;
}
void MainWindow::updateSelectedWorkOrder()
{
    const int row = m_workOrdersTable ? m_workOrdersTable->currentRow() : -1;
    if (row < 0 || !m_workOrdersTable->item(row, 0)) {
        showPlainMessage(this, "Select a work order", "Choose one of your assigned jobs first.");
        return;
    }
    const qint64 id = m_workOrdersTable->item(row, 0)->data(Qt::UserRole).toLongLong();
    QString error;
    if (!m_workOrders.update(id, m_workStatusCombo->currentText(),
                             m_workPerformedEdit->toPlainText(),
                             m_partsUsedEdit->text(), &error)) {
        showPlainMessage(this, "Could not save progress", error);
        return;
    }
    refreshWorkOrders();
    refreshHomeSummary();
    refreshMaintenanceReports();
    refreshParts();
    showSuccess("Progress saved", "The work order and fault status have been updated.");
}
void MainWindow::requestPartsForSelectedWorkOrder()
{
    const int row = m_workOrdersTable ? m_workOrdersTable->currentRow() : -1;
    if (row < 0 || !m_workOrdersTable->item(row, 0)) {
        showPlainMessage(this, "Select a work order", "Choose an assigned job before requesting parts.");
        return;
    }
    const qint64 id = m_workOrdersTable->item(row, 0)->data(Qt::UserRole).toLongLong();
    QString error;
    if (!m_workOrders.requestParts(id, m_partsRequestEdit->text(), &error)) {
        showPlainMessage(this, "Could not request parts", error);
        return;
    }
    refreshWorkOrders();
    refreshParts();
    refreshHomeSummary();
    refreshMaintenanceReports();
    showSuccess("Parts request sent", "The request was saved and the work order is marked Awaiting Parts.");
}
void MainWindow::refreshParts()
{
    if (!m_partsTable) return;
    QString error;
    const QList<WorkOrder> orders = m_workOrders.all(m_username, &error);
    if (!error.isEmpty()) {
        statusBar()->showMessage(error, 6000);
        return;
    }
    m_partsTable->setRowCount(orders.size());
    for (int row = 0; row < orders.size(); ++row) {
        const WorkOrder &order = orders.at(row);
        m_partsTable->setItem(row, 0, new QTableWidgetItem(QString("WO-%1").arg(order.id, 6, 10, QChar('0'))));
        m_partsTable->setItem(row, 1, new QTableWidgetItem(order.campus + " | " + order.building + " / " + order.room));
        m_partsTable->setItem(row, 2, new QTableWidgetItem(order.status));
        const QString details = "Requested: " + (order.partsRequested.isEmpty() ? QStringLiteral("None") : order.partsRequested)
            + "\nUsed: " + (order.partsUsed.isEmpty() ? QStringLiteral("None") : order.partsUsed);
        m_partsTable->setItem(row, 3, new QTableWidgetItem(details));
    }
}
