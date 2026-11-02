#include "ui/MainWindow.h"
#include "ui/UiHelpers.h"

#include "data/FaultRepository.h"
#include "domain/FaultReport.h"
#include "domain/FaultRules.h"

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QStatusBar>
#include <QVBoxLayout>

using namespace UiHelpers;
QWidget *MainWindow::createReportFormPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(18);

    layout->addWidget(makeLabel("Fault details", "sectionTitle", page));
    auto *reportGroup = new QGroupBox(page);
    auto *form = new QGridLayout(reportGroup);
    form->setHorizontalSpacing(16);
    form->setVerticalSpacing(16);
    m_buildingEdit = new QLineEdit(reportGroup);
    m_buildingEdit->setPlaceholderText("For example, Main Library");
    m_roomEdit = new QLineEdit(reportGroup);
    m_roomEdit->setPlaceholderText("For example, Room 204");
    m_campusCombo = new QComboBox(reportGroup);
    m_campusCombo->addItems({"Main Campus", "MP Campus"});
    m_categoryCombo = new QComboBox(reportGroup);
    m_categoryCombo->addItems({"Electrical", "Plumbing", "Furniture", "IT / AV", "Safety", "Other"});
    m_urgencyCombo = new QComboBox(reportGroup);
    m_urgencyCombo->addItems({"Emergency", "Urgent", "Normal", "Low"});
    QSettings settings;
    m_campusCombo->setCurrentText(settings.value("defaults/campus", "Main Campus").toString());
    m_urgencyCombo->setCurrentText(settings.value("defaults/urgency", "Normal").toString());
    m_descriptionEdit = new QPlainTextEdit(reportGroup);
    m_descriptionEdit->setPlaceholderText("Describe the fault and where it can be found (10 to 1000 characters).");
    m_descriptionEdit->setMinimumHeight(130);

    form->addWidget(new QLabel("Campus", reportGroup), 0, 0);
    form->addWidget(m_campusCombo, 0, 1);
    form->addWidget(new QLabel("Building", reportGroup), 0, 2);
    form->addWidget(m_buildingEdit, 0, 3);
    form->addWidget(new QLabel("Room", reportGroup), 1, 0);
    form->addWidget(m_roomEdit, 1, 1);
    form->addWidget(new QLabel("Category", reportGroup), 1, 2);
    form->addWidget(m_categoryCombo, 1, 3);
    form->addWidget(new QLabel("Urgency", reportGroup), 2, 0);
    form->addWidget(m_urgencyCombo, 2, 1);
    form->addWidget(new QLabel("Description", reportGroup), 3, 0);
    form->addWidget(m_descriptionEdit, 3, 1, 1, 3);
    form->setColumnStretch(1, 1);
    form->setColumnStretch(3, 1);
    auto *submit = new QPushButton("Submit fault report", reportGroup);
    submit->setObjectName("primaryButton");
    submit->setMinimumHeight(46);
    form->addWidget(submit, 4, 3);
    layout->addWidget(reportGroup);
    layout->addStretch(1);
    connect(submit, &QPushButton::clicked, this, &MainWindow::submitFault);
    return page;
}
void MainWindow::submitFault()
{
    if (m_role != "Reporter") {
        statusBar()->showMessage("Your account role cannot submit fault reports.", 5000);
        return;
    }
    FaultReport report;
    report.building = m_buildingEdit->text();
    report.room = m_roomEdit->text();
    report.campus = m_campusCombo->currentText();
    report.category = m_categoryCombo->currentText();
    report.description = m_descriptionEdit->toPlainText();
    report.urgency = m_urgencyCombo->currentText();
    report.reportedBy = m_username;

    const QString validationError = FaultRules::validationError(report);
    if (!validationError.isEmpty()) {
        showPlainMessage(this, "Check the report", validationError);
        return;
    }
    report.priority = FaultRules::priorityForUrgency(report.urgency);
    QString error;
    if (!m_repository.add(report, &error)) {
        showPlainMessage(this, "Could not save report", error);
        return;
    }

    m_buildingEdit->clear();
    m_roomEdit->clear();
    m_campusCombo->setCurrentText("Main Campus");
    m_descriptionEdit->clear();
    m_categoryCombo->setCurrentIndex(0);
    m_urgencyCombo->setCurrentText("Normal");
    statusBar()->showMessage(QString("Fault FR-%1 saved").arg(report.id, 6, 10, QChar('0')), 5000);
    refreshHomeSummary();
    openPage("my-reports");
    showSuccess("Fault report sent", QString("Your report FR-%1 has been submitted and your reports list is up to date.")
                    .arg(report.id, 6, 10, QChar('0')));
}
