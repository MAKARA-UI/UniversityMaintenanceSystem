#include "MainWindow.h"

#include "data/AccountRepository.h"
#include "data/FaultRepository.h"
#include "data/WorkOrderRepository.h"
#include "domain/FaultReport.h"
#include "UiHelpers.h"

#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStringList>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>

#include <utility>

using namespace UiHelpers;
MainWindow::MainWindow(FaultRepository &repository, AccountRepository &accounts,
                       WorkOrderRepository &workOrders, LocationRepository &locations,
                       QString username, QString role,
                       std::function<void()> logoutHandler, QWidget *parent)
    : QMainWindow(parent), m_repository(repository), m_accounts(accounts),
      m_workOrders(workOrders),
      m_locations(locations),
      m_username(std::move(username)), m_role(std::move(role)),
      m_logoutHandler(std::move(logoutHandler))
{
    if (!AccountRepository::availableRoles().contains(m_role)) {
        m_role = "Reporter";
    }
    setWindowTitle("Campus Care | University Maintenance");
    setMinimumSize(1000, 680);
    resize(1280, 820);

    auto *central = new QWidget(this);
    if (m_role == "Administrator") central->setObjectName("dashboardAdministrator");
    else if (m_role == "Facilities manager") central->setObjectName("dashboardFacilitiesManager");
    else if (m_role == "Technician") central->setObjectName("dashboardTechnician");
    else central->setObjectName("dashboardReporter");
    auto *shell = new QHBoxLayout(central);
    shell->setContentsMargins(0, 0, 0, 0);
    shell->setSpacing(0);

    auto *sidebar = new QWidget(central);
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(248);
    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(20, 24, 20, 20);
    sidebarLayout->setSpacing(10);

    auto *brand = makeLabel("CAMPUS CARE", "sidebarBrand", sidebar);
    sidebarLayout->addWidget(brand);
    auto *productName = makeLabel("Facilities maintenance", "sidebarSubtitle", sidebar);
    sidebarLayout->addWidget(productName);
    sidebarLayout->addSpacing(24);
    auto *navCaption = makeLabel("WORKSPACE", "sidebarCaption", sidebar);
    sidebarLayout->addWidget(navCaption);

    m_pages = new QStackedWidget(central);
    const auto addPage = [this](const QString &key, QWidget *page) {
        m_pageIndexes.insert(key, m_pages->addWidget(page));
    };
    addPage("home", createHomePage());

    if (m_role == "Reporter") {
        addPage("report", createReportFormPage());
        addPage("my-reports", createReportsPage(
            "My reports", "Faults you have submitted and their latest status."));
    } else if (m_role == "Facilities manager") {
        addPage("faults", createReportsPage(
            "Fault queue", "Review campus faults and prepare maintenance work."));
        addPage("work-orders", createManagerWorkOrdersPage());
        addPage("technicians", createTechniciansPage());
        addPage("reports", createMaintenanceReportsPage());
    } else if (m_role == "Technician") {
        addPage("work-orders", createTechnicianWorkOrdersPage());
        addPage("inventory", createPartsPage());
    } else if (m_role == "Administrator") {
        addPage("faults", createReportsPage(
            "All fault reports", "View the reports recorded in this local project database."));
        addPage("users", createUsersPage());
        addPage("buildings", createLocationsPage());
        addPage("technicians", createTechniciansPage());
        addPage("settings", createSettingsPage());
    } else {
        // Unknown roles receive the least-privileged reporter workspace.
        m_role = "Reporter";
        addPage("report", createReportFormPage());
        addPage("my-reports", createReportsPage(
            "My reports", "Faults you have submitted and their latest status."));
    }

    addNavigationItem("home", "Home");
    if (m_role == "Reporter") {
        addNavigationItem("report", "Report a fault");
        addNavigationItem("my-reports", "My reports");
    } else if (m_role == "Facilities manager") {
        addNavigationItem("faults", "Fault queue");
        addNavigationItem("work-orders", "Work orders");
        addNavigationItem("technicians", "Technicians");
        addNavigationItem("reports", "Maintenance reports");
    } else if (m_role == "Technician") {
        addNavigationItem("work-orders", "My work orders");
        addNavigationItem("inventory", "Parts inventory");
    } else {
        addNavigationItem("faults", "All fault reports");
        addNavigationItem("users", "User accounts");
        addNavigationItem("buildings", "Buildings and rooms");
        addNavigationItem("technicians", "Technicians");
        addNavigationItem("settings", "System settings");
    }

    sidebarLayout->addStretch(1);
    auto *profile = new QFrame(sidebar);
    profile->setObjectName("profilePanel");
    auto *profileLayout = new QVBoxLayout(profile);
    profileLayout->setContentsMargins(12, 12, 12, 12);
    profileLayout->setSpacing(4);
    profileLayout->addWidget(makeLabel(m_username, "profileName", profile));
    profileLayout->addWidget(makeLabel(m_role, "profileRole", profile));
    sidebarLayout->addWidget(profile);
    auto *logoutButton = new QPushButton("Log out", sidebar);
    logoutButton->setObjectName("logoutButton");
    logoutButton->setMinimumHeight(44);
    sidebarLayout->addWidget(logoutButton);
    connect(logoutButton, &QPushButton::clicked, this, [this] {
        close();
        if (m_logoutHandler) m_logoutHandler();
    });

    auto *content = new QWidget(central);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(32, 26, 32, 24);
    contentLayout->setSpacing(14);
    m_pageHeading = makeLabel("Home", "pageTitle", content);
    contentLayout->addWidget(m_pageHeading);
    m_pageSubtitle = makeLabel("Your maintenance workspace at a glance.", "pageSubtitle", content);
    contentLayout->addWidget(m_pageSubtitle);
    contentLayout->addWidget(m_pages, 1);

    shell->addWidget(sidebar);
    shell->addWidget(content, 1);
    setCentralWidget(central);
    statusBar()->showMessage("Signed in as " + m_username + " | " + m_role);
    openPage("home");
}
void MainWindow::addNavigationItem(const QString &key, const QString &label)
{
    auto *button = new QPushButton(label, findChild<QWidget *>("sidebar"));
    button->setObjectName("sidebarButton");
    button->setCheckable(true);
    button->setMinimumHeight(44);
    auto *sidebar = findChild<QWidget *>("sidebar");
    sidebar->layout()->addWidget(button);
    m_navigationButtons.insert(key, button);
    connect(button, &QPushButton::clicked, this, [this, key] { openPage(key); });
}
void MainWindow::openPage(const QString &key)
{
    const auto found = m_pageIndexes.constFind(key);
    if (found == m_pageIndexes.cend()) {
        return; // A role cannot open a page outside its navigation set.
    }
    m_pages->setCurrentIndex(found.value());
    if (key == "home") refreshHomeSummary();
    for (auto it = m_navigationButtons.cbegin(); it != m_navigationButtons.cend(); ++it) {
        it.value()->setChecked(it.key() == key);
    }

    QString heading = "Home";
    QString subtitle = "Your maintenance workspace at a glance.";
    if (key == "home") {
        heading = m_role + " dashboard";
        if (m_role == "Administrator") subtitle = "Manage system access and review the campus maintenance records.";
        else if (m_role == "Facilities manager") subtitle = "Review incoming faults and coordinate maintenance activity.";
        else if (m_role == "Technician") subtitle = "Keep track of assigned repairs and maintenance resources.";
        else subtitle = "Report a facilities problem and follow its progress.";
    }
    if (key == "report") { heading = "Report a fault"; subtitle = "Give the maintenance team clear details about the problem."; }
    if (key == "my-reports") { heading = "My reports"; subtitle = "Follow the status of faults you submitted."; refreshReports(); }
    if (key == "faults") { heading = m_role == "Administrator" ? "All fault reports" : "Fault queue"; subtitle = "Review reported facilities problems."; refreshReports(); }
    if (key == "work-orders") { heading = m_role == "Technician" ? "My work orders" : "Work orders"; subtitle = "Maintenance assignments and repair progress."; refreshWorkOrders(); }
    if (key == "inventory") { heading = "Parts requests and usage"; subtitle = "Review spare parts requested and recorded against your assigned work."; refreshParts(); }
    if (key == "reports") { heading = "Maintenance reports"; subtitle = "Maintenance activity and service summaries."; refreshMaintenanceReports(); }
    if (key == "users") { heading = "User accounts"; subtitle = "Review accounts and set their application roles."; refreshUsers(); }
    if (key == "buildings") { heading = "Buildings and rooms"; subtitle = "Register locations for Main Campus and MP Campus."; refreshLocations(); }
    if (key == "technicians") { heading = "Technicians"; subtitle = m_role == "Administrator" ? "Review technician accounts and their activation status." : "Active technicians available for maintenance assignments."; refreshTechnicians(); }
    if (key == "settings") { heading = "System settings"; subtitle = "Choose the default campus and urgency for new fault reports."; }
    m_pageHeading->setText(heading);
    m_pageSubtitle->setText(subtitle);
}
QWidget *MainWindow::createHomePage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 12, 0, 0);
    layout->setSpacing(22);

    const QString roleTitle = m_role == "Administrator" ? "Administrator overview"
        : m_role == "Facilities manager" ? "Facilities overview"
        : m_role == "Technician" ? "Technician workspace" : "Your campus reports";
    auto *welcome = makeLabel("Welcome, " + m_username + "  |  " + roleTitle,
                              "welcomeTitle", page);
    layout->addWidget(welcome);
    auto *intro = makeLabel(permissionSummary(m_role) + ". Your home page and navigation are tailored to this account role.",
                            "pageSubtitle", page);
    layout->addWidget(intro);

    auto *cards = new QHBoxLayout;
    cards->setSpacing(16);
    const auto addCard = [cards, page](const QString &label, const QString &value,
                                       const QString &detail) {
        auto *card = new QFrame(page);
        card->setObjectName("summaryCard");
        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(20, 18, 20, 18);
        cardLayout->setSpacing(8);
        cardLayout->addWidget(makeLabel(label, "cardLabel", card));
        cardLayout->addWidget(makeLabel(value, "cardValue", card));
        cardLayout->addWidget(makeLabel(detail, "cardDetail", card));
        cards->addWidget(card, 1);
    };
    if (m_role == "Reporter") {
        addCard("MY FAULT REPORTS", "0", "Submitted by your account");
        m_homeTotalValue = page->findChild<QLabel *>("cardValue");
        addCard("OPEN REPORTS", "0", "Awaiting resolution or closure");
    } else if (m_role == "Facilities manager") {
        addCard("FAULTS IN QUEUE", "0", "Reported campus problems");
        m_homeTotalValue = page->findChild<QLabel *>("cardValue");
        addCard("OPEN FAULTS", "0", "Need review or maintenance");
    } else if (m_role == "Administrator") {
        QString accountError;
        const auto users = m_accounts.allAccounts(&accountError);
        addCard("USER ACCOUNTS", QString::number(users.size()), "Accounts and assigned roles");
        addCard("FAULT RECORDS", "0", "Campus reports in the system");
        m_homeTotalValue = page->findChildren<QLabel *>("cardValue").value(1, nullptr);
    } else {
        const auto orders = m_workOrders.all(m_username);
        int activeOrders = 0;
        for (const WorkOrder &order : orders) {
            if (order.status == "In Progress" || order.status == "Accepted") ++activeOrders;
        }
        addCard("ASSIGNED WORK", QString::number(orders.size()), "Work orders assigned to your account");
        m_homeTotalValue = page->findChildren<QLabel *>("cardValue").last();
        addCard("ACTIVE REPAIRS", QString::number(activeOrders), "Accepted or in progress");
        m_homeOpenValue = page->findChildren<QLabel *>("cardValue").last();
    }
    if (m_role != "Administrator" && m_role != "Technician") {
        addCard(m_role == "Reporter" ? "YOUR ROLE" : "SERVICE AREA",
                m_role == "Reporter" ? "Reporter" : "Campus",
                m_role == "Reporter" ? "View is limited to your reports" : "Fault queue access is enabled");
    } else if (m_role == "Administrator") {
        addCard("ACCESS CONTROL", "Roles", "Administrator can manage account roles");
    } else {
        addCard("PARTS", "Requests", "Record parts needed or used with your work order");
    }
    const QList<QLabel *> values = page->findChildren<QLabel *>("cardValue");
    if (m_role == "Reporter" || m_role == "Facilities manager") {
        if (values.size() > 1) m_homeOpenValue = values.at(1);
    }
    layout->addLayout(cards);

    auto *tip = new QFrame(page);
    tip->setObjectName("infoPanel");
    auto *tipLayout = new QVBoxLayout(tip);
    tipLayout->setContentsMargins(22, 20, 22, 20);
    tipLayout->setSpacing(8);
    tipLayout->addWidget(makeLabel("Your responsibilities", "sectionTitle", tip));
    QString responsibility;
    if (m_role == "Administrator") responsibility = "Manage users, campus locations, technician records, and system settings.";
    else if (m_role == "Facilities manager") responsibility = "Review and prioritise faults, assign technicians, oversee repairs, and review reports.";
    else if (m_role == "Technician") responsibility = "Handle assigned work, update repair progress, record work performed, and account for parts.";
    else responsibility = "Submit a fault with its building, room, description, and urgency, then track its status.";
    tipLayout->addWidget(makeLabel(responsibility, "cardDetail", tip));
    layout->addWidget(tip);

    auto *quickActions = new QHBoxLayout;
    quickActions->setSpacing(12);
    const auto addAction = [this, page, quickActions](const QString &key, const QString &text) {
        auto *button = new QPushButton(text, page);
        button->setObjectName("quickActionButton");
        button->setMinimumHeight(46);
        quickActions->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, key] { openPage(key); });
    };
    if (m_role == "Reporter") {
        addAction("report", "Submit a fault");
        addAction("my-reports", "View my reports");
    } else if (m_role == "Facilities manager") {
        addAction("faults", "Review fault queue");
        addAction("work-orders", "Open work orders");
    } else if (m_role == "Technician") {
        addAction("work-orders", "View my work orders");
        addAction("inventory", "View parts inventory");
    } else {
        addAction("users", "Manage user roles");
        addAction("faults", "View fault reports");
    }
    layout->addLayout(quickActions);
    layout->addStretch(1);
    refreshHomeSummary();
    return page;
}
void MainWindow::refreshHomeSummary()
{
    if (m_role == "Technician") {
        const auto orders = m_workOrders.all(m_username);
        int activeOrders = 0;
        for (const WorkOrder &order : orders) {
            if (order.status == "In Progress" || order.status == "Accepted") ++activeOrders;
        }
        if (m_homeTotalValue) m_homeTotalValue->setText(QString::number(orders.size()));
        if (m_homeOpenValue) m_homeOpenValue->setText(QString::number(activeOrders));
        return;
    }
    QString error;
    const QString reporter = m_role == "Reporter" ? m_username : QString();
    const QList<FaultReport> reports = m_repository.latest(200, &error, reporter);
    int openCount = 0;
    for (const FaultReport &report : reports) {
        if (report.status != "Resolved" && report.status != "Closed") ++openCount;
    }
    if (m_homeTotalValue) m_homeTotalValue->setText(QString::number(reports.size()));
    if (m_homeOpenValue) m_homeOpenValue->setText(QString::number(openCount));
}
QWidget *MainWindow::createPlaceholderPage(const QString &, const QString &description)
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 8, 0, 0);
    auto *panel = new QFrame(page);
    panel->setObjectName("infoPanel");
    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(28, 28, 28, 28);
    panelLayout->setSpacing(12);
    panelLayout->addWidget(makeLabel("This module is planned", "sectionTitle", panel));
    panelLayout->addWidget(makeLabel(description, "cardDetail", panel));
    layout->addWidget(panel);
    layout->addStretch(1);
    return page;
}
void MainWindow::showSuccess(const QString &title, const QString &message)
{
    showPlainMessage(this, title, message);
}
