#pragma once

#include <QString>

struct FaultReport
{
    qint64 id = 0;
    QString building;
    QString room;
    QString campus = "Main Campus";
    QString category;
    QString description;
    QString urgency;
    QString priority;
    QString status = "Submitted";
    QString reportedAt;
    QString reportedBy;
};
