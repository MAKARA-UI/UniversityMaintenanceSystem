#include "FaultRules.h"

#include <QStringList>

QString FaultRules::validationError(const FaultReport &report)
{
    const QString building = report.building.trimmed();
    const QString room = report.room.trimmed();
    const QString description = report.description.trimmed();

    if (building.isEmpty()) {
        return "Building is required.";
    }
    if (building.size() > 100) {
        return "Building must be 100 characters or fewer.";
    }
    if (room.isEmpty()) {
        return "Room is required.";
    }
    if (room.size() > 50) {
        return "Room must be 50 characters or fewer.";
    }

    const QStringList categories{
        "Electrical", "Plumbing", "Furniture", "IT / AV", "Safety", "Other"
    };
    if (!categories.contains(report.category)) {
        return "Select a valid fault category.";
    }

    const QStringList urgencies{"Emergency", "Urgent", "Normal", "Low"};
    if (!urgencies.contains(report.urgency)) {
        return "Select a valid urgency.";
    }

    if (description.size() < 10) {
        return "Description must contain at least 10 characters.";
    }
    if (description.size() > 1000) {
        return "Description must be 1000 characters or fewer.";
    }

    return {};
}

QString FaultRules::priorityForUrgency(const QString &urgency)
{
    if (urgency == "Emergency") {
        return "Critical";
    }
    if (urgency == "Urgent") {
        return "High";
    }
    if (urgency == "Low") {
        return "Low";
    }
    return "Medium";
}
