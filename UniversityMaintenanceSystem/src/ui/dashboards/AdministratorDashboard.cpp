#include "ui/MainWindow.h"
#include "ui/UiHelpers.h"

#include "data/AccountRepository.h"
#include "data/LocationRepository.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QStatusBar>
#include <QTimer>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

using namespace UiHelpers;
QWidget *MainWindow::createUsersPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(12);
    layout->addWidget(makeLabel(
        "Create accounts, register technicians by choosing the Technician role, change roles, and activate or deactivate accounts.",
        "infoText", page));
    layout->addWidget(makeLabel("Create user or technician account", "sectionTitle", page));
    auto *createBox = new QGroupBox(page);
    auto *createForm = new QGridLayout(createBox);
    auto *newUsername = new QLineEdit(createBox);
    newUsername->setPlaceholderText("Username");
    auto *newPassword = new QLineEdit(createBox);
    newPassword->setPlaceholderText("Temporary password (at least 10 characters)");
    newPassword->setEchoMode(QLineEdit::Password);
    auto *newRole = new QComboBox(createBox);
    newRole->addItems(AccountRepository::availableRoles());
    auto *createButton = new QPushButton("Create account", createBox);
    createButton->setObjectName("primaryButton");
    createForm->addWidget(new QLabel("Username", createBox), 0, 0);
    createForm->addWidget(newUsername, 0, 1);
    createForm->addWidget(new QLabel("Password", createBox), 0, 2);
    createForm->addWidget(newPassword, 0, 3);
    createForm->addWidget(new QLabel("Role", createBox), 1, 0);
    createForm->addWidget(newRole, 1, 1);
    createForm->addWidget(createButton, 1, 3);
    layout->addWidget(createBox);
    connect(createButton, &QPushButton::clicked, this,
            [this, newUsername, newPassword, newRole] {
        if (newUsername->text().trimmed().size() < 3 || newPassword->text().size() < 10) {
            showPlainMessage(this, "Check account details",
                             "Use a username with at least 3 characters and a password with at least 10 characters.");
            return;
        }
        QString error;
        if (!m_accounts.createAccount(newUsername->text(), newPassword->text(),
                                      newRole->currentText(), &error)) {
            showPlainMessage(this, "Account not created", error);
            return;
        }
        newUsername->clear();
        newPassword->clear();
        refreshUsers();
        showSuccess("Account created", "The account is ready. Share its sign-in details securely with the user.");
    });
    m_usersTable = new QTableWidget(0, 4, page);
    m_usersTable->setHorizontalHeaderLabels({"Username", "Role", "Active", "Access summary"});
    m_usersTable->verticalHeader()->setVisible(false);
    m_usersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_usersTable->setAlternatingRowColors(true);
    m_usersTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_usersTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_usersTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_usersTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    layout->addWidget(m_usersTable, 1);
    return page;
}
QWidget *MainWindow::createLocationsPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(12);
    layout->addWidget(makeLabel("Register each building and room under the correct campus. Reporters can select the campus when submitting a fault.", "infoText", page));
    layout->addWidget(makeLabel("Register a building room", "sectionTitle", page));
    auto *box = new QGroupBox(page);
    auto *form = new QGridLayout(box);
    m_locationCampusCombo = new QComboBox(box);
    m_locationCampusCombo->addItems({"Main Campus", "MP Campus"});
    m_locationBuildingEdit = new QLineEdit(box);
    m_locationBuildingEdit->setPlaceholderText("Building name");
    m_locationRoomEdit = new QLineEdit(box);
    m_locationRoomEdit->setPlaceholderText("Room number or name");
    auto *add = new QPushButton("Register location", box);
    add->setObjectName("primaryButton");
    form->addWidget(new QLabel("Campus", box), 0, 0);
    form->addWidget(m_locationCampusCombo, 0, 1);
    form->addWidget(new QLabel("Building", box), 0, 2);
    form->addWidget(m_locationBuildingEdit, 0, 3);
    form->addWidget(new QLabel("Room", box), 1, 0);
    form->addWidget(m_locationRoomEdit, 1, 1);
    form->addWidget(add, 1, 3);
    layout->addWidget(box);
    m_locationsTable = new QTableWidget(0, 3, page);
    m_locationsTable->setHorizontalHeaderLabels({"Campus", "Building", "Room"});
    m_locationsTable->verticalHeader()->setVisible(false);
    m_locationsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_locationsTable->setAlternatingRowColors(true);
    m_locationsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(m_locationsTable, 1);
    connect(add, &QPushButton::clicked, this, [this] {
        if (m_locationBuildingEdit->text().trimmed().isEmpty() ||
            m_locationRoomEdit->text().trimmed().isEmpty()) {
            showPlainMessage(this, "Location details needed", "Enter both a building and a room.");
            return;
        }
        QString error;
        if (!m_locations.add(m_locationCampusCombo->currentText(), m_locationBuildingEdit->text(),
                             m_locationRoomEdit->text(), &error)) {
            showPlainMessage(this, "Location not registered", error);
            return;
        }
        m_locationBuildingEdit->clear();
        m_locationRoomEdit->clear();
        refreshLocations();
        showSuccess("Location registered", "The campus building and room have been saved.");
    });
    return page;
}
QWidget *MainWindow::createTechniciansPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->addWidget(makeLabel(m_role == "Administrator"
        ? "Technician accounts are created in User accounts by selecting the Technician role. This list shows active technicians."
        : "Active technicians available for work assignments. Select a fault in the queue and use Create work order to assign one.",
        "infoText", page));
    m_techniciansTable = new QTableWidget(0, 2, page);
    m_techniciansTable->setHorizontalHeaderLabels({"Technician username", "Account status"});
    m_techniciansTable->verticalHeader()->setVisible(false);
    m_techniciansTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_techniciansTable->setAlternatingRowColors(true);
    m_techniciansTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(m_techniciansTable, 1);
    return page;
}
QWidget *MainWindow::createSettingsPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->addWidget(makeLabel("Fault report defaults", "sectionTitle", page));
    auto *box = new QGroupBox(page);
    auto *form = new QFormLayout(box);
    m_settingsCampusCombo = new QComboBox(box);
    m_settingsCampusCombo->addItems({"Main Campus", "MP Campus"});
    m_settingsUrgencyCombo = new QComboBox(box);
    m_settingsUrgencyCombo->addItems({"Emergency", "Urgent", "Normal", "Low"});
    QSettings settings;
    m_settingsCampusCombo->setCurrentText(settings.value("defaults/campus", "Main Campus").toString());
    m_settingsUrgencyCombo->setCurrentText(settings.value("defaults/urgency", "Normal").toString());
    form->addRow("Default campus", m_settingsCampusCombo);
    form->addRow("Default urgency", m_settingsUrgencyCombo);
    auto *save = new QPushButton("Save system settings", box);
    save->setObjectName("primaryButton");
    form->addRow(save);
    layout->addWidget(box);
    layout->addWidget(makeLabel("These preferences fill in the starting values for a new fault report. Reporters can still choose a different campus and urgency.", "infoText", page));
    layout->addStretch(1);
    connect(save, &QPushButton::clicked, this, [this] {
        QSettings settings;
        settings.setValue("defaults/campus", m_settingsCampusCombo->currentText());
        settings.setValue("defaults/urgency", m_settingsUrgencyCombo->currentText());
        showSuccess("Settings saved", "New fault reports will use these default values.");
    });
    return page;
}
void MainWindow::refreshUsers()
{
    if (!m_usersTable) return;
    QString error;
    const QList<UserAccount> accounts = m_accounts.allAccounts(&error);
    if (!error.isEmpty()) {
        statusBar()->showMessage(error);
        return;
    }
    m_usersTable->setRowCount(accounts.size());
    const QStringList roles = AccountRepository::availableRoles();
    for (int row = 0; row < accounts.size(); ++row) {
        const UserAccount &account = accounts.at(row);
        auto *username = new QTableWidgetItem(account.username);
        username->setData(Qt::UserRole, account.id);
        m_usersTable->setItem(row, 0, username);
        auto *roleBox = new QComboBox(m_usersTable);
        roleBox->addItems(roles);
        roleBox->setCurrentText(account.role);
        roleBox->setEnabled(account.username.compare(m_username, Qt::CaseInsensitive) != 0);
        m_usersTable->setCellWidget(row, 1, roleBox);
        auto *activeCheck = new QCheckBox(m_usersTable);
        activeCheck->setChecked(account.active);
        activeCheck->setEnabled(account.username.compare(m_username, Qt::CaseInsensitive) != 0);
        m_usersTable->setCellWidget(row, 2, activeCheck);
        m_usersTable->setItem(row, 3, new QTableWidgetItem(permissionSummary(account.role)));
        connect(roleBox, &QComboBox::currentTextChanged, this,
                [this, account, row](const QString &newRole) {
            if (newRole == account.role) return;
            QString updateError;
            if (!m_accounts.updateRole(account.id, newRole, &updateError)) {
                statusBar()->showMessage(updateError.isEmpty() ? "Role was not changed." : updateError, 5000);
                refreshUsers();
                return;
            }
            m_usersTable->item(row, 3)->setText(permissionSummary(newRole));
            showSuccess("Account role updated", "The role for " + account.username + " has been changed.");
            QTimer::singleShot(0, this, [this] { refreshUsers(); });
        });
        connect(activeCheck, &QCheckBox::toggled, this,
                [this, account](bool active) {
            QString updateError;
            if (!m_accounts.setActive(account.id, active, &updateError)) {
                statusBar()->showMessage(updateError.isEmpty() ? "Account status was not changed." : updateError, 5000);
                refreshUsers();
                return;
            }
            showSuccess(active ? "Account activated" : "Account deactivated",
                        account.username + (active ? " can sign in again." : " can no longer sign in."));
            QTimer::singleShot(0, this, [this] { refreshUsers(); });
        });
    }
}
void MainWindow::refreshLocations()
{
    if (!m_locationsTable) return;
    QString error;
    const QList<CampusRoom> locations = m_locations.all(&error);
    if (!error.isEmpty()) {
        statusBar()->showMessage(error, 6000);
        return;
    }
    m_locationsTable->setRowCount(locations.size());
    for (int row = 0; row < locations.size(); ++row) {
        const CampusRoom &location = locations.at(row);
        m_locationsTable->setItem(row, 0, new QTableWidgetItem(location.campus));
        m_locationsTable->setItem(row, 1, new QTableWidgetItem(location.building));
        m_locationsTable->setItem(row, 2, new QTableWidgetItem(location.room));
    }
}
void MainWindow::refreshTechnicians()
{
    if (!m_techniciansTable) return;
    QString error;
    const QList<UserAccount> accounts = m_accounts.allAccounts(&error);
    if (!error.isEmpty()) {
        statusBar()->showMessage(error, 6000);
        return;
    }
    QList<UserAccount> technicians;
    for (const UserAccount &account : accounts) {
        if (account.role == "Technician" && (account.active || m_role == "Administrator"))
            technicians.append(account);
    }
    m_techniciansTable->setRowCount(technicians.size());
    for (int row = 0; row < technicians.size(); ++row) {
        m_techniciansTable->setItem(row, 0, new QTableWidgetItem(technicians.at(row).username));
        m_techniciansTable->setItem(row, 1,
            new QTableWidgetItem(technicians.at(row).active ? "Active" : "Inactive"));
    }
}
