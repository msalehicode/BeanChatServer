#pragma once

#include <QString>
#include <QDateTime>

struct Message
{
    quint64 id = 0;

    quint64 channelId = 0;
    quint64 senderId = 0;

    int type = 0;

    QString text;

    quint64 attachmentId = 0;

    bool edited = false;
    bool deleted = false;

    QDateTime createdAt;
    QDateTime updatedAt;
};
