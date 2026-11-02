#pragma once

#include "FaultReport.h"

#include <QString>

class FaultRules
{
public:
    static QString validationError(const FaultReport &report);
    static QString priorityForUrgency(const QString &urgency);
};
