#pragma once

#include <QHash>
#include <QMainWindow>

#include <functional>

class AccountRepository;
class FaultRepository;
class WorkOrderRepository;
class LocationRepository;
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QStackedWidget;
class QTableWidget;

class MainWindow final : public QMainWindow
{
public:
    MainWindow(FaultRepository &repository, AccountRepository &accounts,
               WorkOrderRepository &workOrders, LocationRepository &locations,
               QString username, QString role,
               std::function<void()> logoutHandler,
               QWidget *parent = nullptr);

private:
    QWidget *createHomePage();
    QWidget *createReportFormPage();
    QWidget *createReportsPage(const QString &title, const QString &subtitle);
    QWidget *createManagerWorkOrdersPage();
    QWidget *createTechnicianWorkOrdersPage();
    QWidget *createMaintenanceReportsPage();
    QWidget *createUsersPage();
    QWidget *createLocationsPage();
    QWidget *createTechniciansPage();
    QWidget *createSettingsPage();
    QWidget *createPartsPage();
    QWidget *createPlaceholderPage(const QString &title, const QString &subtitle);
    void addNavigationItem(const QString &key, const QString &label);
    void openPage(const QString &key);
    void refreshReports();
    void refreshHomeSummary();
    void refreshUsers();
    void refreshLocations();
    void refreshTechnicians();
    void refreshParts();
    void refreshWorkOrders();
    void refreshMaintenanceReports();
    void assignSelectedFault();
    void closeSelectedWorkOrder();
    void updateSelectedWorkOrder();
    void requestPartsForSelectedWorkOrder();
    void submitFault();
    void showSuccess(const QString &title, const QString &message);

    FaultRepository &m_repository;
    AccountRepository &m_accounts;
    WorkOrderRepository &m_workOrders;
    LocationRepository &m_locations;
    QString m_username;
    QString m_role;
    std::function<void()> m_logoutHandler;
    QStackedWidget *m_pages = nullptr;
    QLabel *m_pageHeading = nullptr;
    QLabel *m_pageSubtitle = nullptr;
    QTableWidget *m_reportsTable = nullptr;
    QTableWidget *m_usersTable = nullptr;
    QTableWidget *m_workOrdersTable = nullptr;
    QTableWidget *m_maintenanceReportsTable = nullptr;
    QTableWidget *m_locationsTable = nullptr;
    QTableWidget *m_techniciansTable = nullptr;
    QTableWidget *m_partsTable = nullptr;
    QLineEdit *m_buildingEdit = nullptr;
    QLineEdit *m_roomEdit = nullptr;
    QComboBox *m_categoryCombo = nullptr;
    QComboBox *m_urgencyCombo = nullptr;
    QComboBox *m_campusCombo = nullptr;
    QComboBox *m_workStatusCombo = nullptr;
    QPlainTextEdit *m_descriptionEdit = nullptr;
    QPlainTextEdit *m_workPerformedEdit = nullptr;
    QLineEdit *m_partsRequestEdit = nullptr;
    QLineEdit *m_partsUsedEdit = nullptr;
    QComboBox *m_locationCampusCombo = nullptr;
    QLineEdit *m_locationBuildingEdit = nullptr;
    QLineEdit *m_locationRoomEdit = nullptr;
    QComboBox *m_settingsCampusCombo = nullptr;
    QComboBox *m_settingsUrgencyCombo = nullptr;
    QLabel *m_reportsSummary = nullptr;
    QLabel *m_homeTotalValue = nullptr;
    QLabel *m_homeOpenValue = nullptr;
    QHash<QString, int> m_pageIndexes;
    QHash<QString, QPushButton *> m_navigationButtons;
};
