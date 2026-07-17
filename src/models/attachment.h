#pragma once

#include <QString>
#include <QByteArray>
#include <QDateTime>

struct Attachment
{
    quint64 id = 0;

    quint64 channelId = 0;
    quint64 uploaderId = 0;

    QString originalFilename;
    QString storedFilename;

    QString mimeType;

    quint64 size = 0;

    QByteArray sha256;

    QDateTime uploadedAt;
};
