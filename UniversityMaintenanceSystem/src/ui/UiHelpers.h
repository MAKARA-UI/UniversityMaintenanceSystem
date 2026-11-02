#pragma once

#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

namespace UiHelpers {

inline QString permissionSummary(const QString &role)
{
    if (role == "Administrator") return "Manage accounts and system setup";
    if (role == "Facilities manager") return "Review faults and coordinate repairs";
    if (role == "Technician") return "Work on assigned maintenance jobs";
    return "Submit faults and track own reports";
}

inline QLabel *makeLabel(const QString &text, const QString &objectName, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName(objectName);
    label->setWordWrap(true);
    return label;
}

inline void showPlainMessage(QWidget *parent, const QString &title, const QString &message)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setModal(true);
    dialog.setMinimumWidth(420);
    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 22, 24, 20);
    layout->setSpacing(18);
    auto *text = new QLabel(message, &dialog);
    text->setWordWrap(true);
    layout->addWidget(text);
    auto *button = new QPushButton("OK", &dialog);
    button->setObjectName("primaryButton");
    button->setMinimumWidth(96);
    layout->addWidget(button, 0, Qt::AlignRight);
    QObject::connect(button, &QPushButton::clicked, &dialog, &QDialog::accept);
    dialog.exec();
}

} // namespace UiHelpers
