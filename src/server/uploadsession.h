#pragma once

#include <QFile>
#include <QString>
#include <QByteArray>

struct UploadSession
{
    quint64 uploadId = 0;

    quint64 channelId = 0;

    QString originalFilename;
    QString mimeType;

    quint64 expectedSize = 0;
    quint64 receivedSize = 0;

    QByteArray sha256;

    QFile file;
};
