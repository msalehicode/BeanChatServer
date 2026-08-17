#pragma once

#include <QString>

enum class TrackSource
{
    Unknown,

    YouTube,

    Playlist,

    Attachment,

    Radio,

    LocalFile
};

struct Track
{
    QString title;
    QString uploader;

    QString originalUrl;
    QString streamUrl;
    QString thumbnailUrl;

    qint64 duration = 0;
    bool live = false;

    bool isValid() const
    {
        return !streamUrl.isEmpty();
    }
};
