#include "ui/MainWindow.h"
#include "ui/UiHelpers.h"

#include "data/FaultRepository.h"
#include "domain/FaultReport.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QHeaderView>
#include <QLabel>
#include <QStatusBar>
#include <QTimer>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QPushButton>

using namespace UiHelpers;
QWidget *MainWindow::createReportsPage(const QString &, const QString &)
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(12);
    m_reportsSummary = makeLabel("", "summaryText", page);
    layout->addWidget(m_reportsSummary);
    if (m_role == "Facilities manager") {
        auto *assign = new QPushButton("Create work order for selected fault", page);
        assign->setObjectName("primaryButton");
        assign->setMinimumHeight(44);
        layout->addWidget(assign, 0, Qt::AlignLeft);
        connect(assign, &QPushButton::clicked, this, &MainWindow::assignSelectedFault);
    }
    m_reportsTable = new QTableWidget(0, 10, page);
    m_reportsTable->setHorizontalHeaderLabels(
        {"Fault ID", "Campus", "Building", "Room", "Category", "Urgency", "Priority", "Status", "Reported", "Submitted by"});
    m_reportsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_reportsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_reportsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_reportsTable->setAlternatingRowColors(true);
    m_reportsTable->verticalHeader()->setVisible(false);
    m_reportsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(m_reportsTable, 1);
    return page;
}
void MainWindow::refreshReports()
{
    if (!m_reportsTable) return;
    QString error;
    const QString reporter = m_role == "Reporter" ? m_username : QString();
    const QList<FaultReport> reports = m_repository.latest(200, &error, reporter);
    if (!error.isEmpty()) {
        statusBar()->showMessage(error);
        return;
    }
    m_reportsTable->setRowCount(reports.size());
    int openCount = 0;
    for (int row = 0; row < reports.size(); ++row) {
        const FaultReport &report = reports.at(row);
        const QStringList values{
            QString("FR-%1").arg(report.id, 6, 10, QChar('0')),
            report.campus, report.building, report.room, report.category, report.urgency,
            report.priority, report.status, report.reportedAt, report.reportedBy};
        for (int column = 0; column < values.size(); ++column) {
            if (column == 6 && m_role == "Facilities manager") {
                auto *priority = new QComboBox(m_reportsTable);
                priority->addItems({"Critical", "High", "Medium", "Low"});
                priority->setCurrentText(report.priority);
                m_reportsTable->setCellWidget(row, column, priority);
                connect(priority, &QComboBox::currentTextChanged, this,
                        [this, reportId = report.id, original = report.priority](const QString &value) {
                    if (value == original) return;
                    QString updateError;
                    if (!m_repository.setPriority(reportId, value, &updateError)) {
                        showPlainMessage(this, "Priority not changed", updateError);
                    } else {
                        refreshHomeSummary();
                        showSuccess("Priority updated", "The fault priority has been saved.");
                        QTimer::singleShot(0, this, [this] { refreshReports(); });
                    }
                });
            } else {
                auto *item = new QTableWidgetItem(values.at(column));
                if (column == 0) item->setData(Qt::UserRole, report.id);
                m_reportsTable->setItem(row, column, item);
            }
        }
        if (report.status != "Resolved" && report.status != "Closed") ++openCount;
    }
    if (m_reportsSummary) {
        m_reportsSummary->setText(QString("%1 reports shown  |  %2 still open")
                                      .arg(reports.size()).arg(openCount));
    }
}
