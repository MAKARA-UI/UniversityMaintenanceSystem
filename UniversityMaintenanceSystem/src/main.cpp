#include "data/FaultRepository.h"
#include "data/AccountRepository.h"
#include "data/WorkOrderRepository.h"
#include "data/LocationRepository.h"
#include "ui/LoginDialog.h"
#include "ui/MainWindow.h"

#include <QApplication>
#include <QDir>
#include <QDialog>
#include <QEventLoop>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStandardPaths>
#include <QStringList>
#include <QVBoxLayout>

namespace {
void showStartupMessage(const QString &title, const QString &message)
{
    QDialog dialog;
    dialog.setWindowTitle(title);
    dialog.setModal(true);
    dialog.setMinimumWidth(420);
    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 22, 24, 20);
    layout->setSpacing(18);
    auto *label = new QLabel(message, &dialog);
    label->setWordWrap(true);
    layout->addWidget(label);
    auto *actions = new QHBoxLayout;
    actions->addStretch(1);
    auto *ok = new QPushButton("OK", &dialog);
    ok->setObjectName("primaryButton");
    ok->setMinimumWidth(96);
    actions->addWidget(ok);
    layout->addLayout(actions);
    QObject::connect(ok, &QPushButton::clicked, &dialog, &QDialog::accept);
    dialog.exec();
}
}

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setQuitOnLastWindowClosed(false);
    QCoreApplication::setOrganizationName("BIST4110Group");
    QCoreApplication::setApplicationName("UniversityMaintenanceSystem");

    const QStringList styleFiles{
        ":/styles/base.qss",
        ":/styles/authentication.qss",
        ":/styles/sidebar.qss",
        ":/styles/dashboard.qss",
        ":/styles/dashboards/AdministratorDashboard.qss",
        ":/styles/dashboards/FacilitiesManagerDashboard.qss",
        ":/styles/dashboards/TechnicianDashboard.qss",
        ":/styles/dashboards/ReporterDashboard.qss",
        ":/styles/forms.qss",
        ":/styles/tables.qss",
        ":/styles/dialogs.qss"
    };
    QString combinedStyle;
    for (const QString &stylePath : styleFiles) {
        QFile stylesheet(stylePath);
        if (!stylesheet.open(QIODevice::ReadOnly | QIODevice::Text)) {
            showStartupMessage("Style resource error", "Could not load " + stylePath);
            return 1;
        }
        combinedStyle += QString::fromUtf8(stylesheet.readAll());
        combinedStyle += '\n';
    }
    application.setStyleSheet(combinedStyle);

    const QString dataDirectory =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!QDir().mkpath(dataDirectory)) {
        showStartupMessage("Startup error", "The application could not create its data folder.");
        return 1;
    }

    FaultRepository repository(dataDirectory + "/maintenance.sqlite");
    AccountRepository accounts(dataDirectory + "/maintenance.sqlite");
    WorkOrderRepository workOrders(dataDirectory + "/maintenance.sqlite");
    LocationRepository locations(dataDirectory + "/maintenance.sqlite");
    QString error;
    if (!repository.initialize(&error) || !accounts.initialize(&error) ||
        !workOrders.initialize(&error) || !locations.initialize(&error)) {
        showStartupMessage("Database error", error);
        return 1;
    }

    while (true) {
        LoginDialog login(accounts);
        if (login.exec() != QDialog::Accepted) {
            break;
        }

        QEventLoop sessionLoop;
        bool logoutRequested = false;
        MainWindow window(repository, accounts, workOrders, locations,
                          login.username(), login.role(), [&] {
            logoutRequested = true;
            sessionLoop.quit();
        });
        QObject::connect(&application, &QApplication::lastWindowClosed,
                         &sessionLoop, &QEventLoop::quit);
        window.show();
        sessionLoop.exec();
        if (!logoutRequested) {
            break; // Closing the dashboard window exits the application.
        }
    }
    return 0;
}
