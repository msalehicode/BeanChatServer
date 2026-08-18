#include "youtubeprovider.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDebug>

YoutubeProvider::YoutubeProvider(QObject *parent)
    : QObject(parent)
{
}

void YoutubeProvider::search(const QString &query)
{
    if (m_process.state() != QProcess::NotRunning)
        m_process.kill();

    m_query = query;
    m_track = Track();

    disconnect(&m_process, nullptr, this, nullptr);

    connect(&m_process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            &YoutubeProvider::onSearchFinished);

    QStringList args;

    args
        << "--no-warnings"
        << "--dump-single-json"
        << "--no-playlist"
        << QString("ytsearch1:%1").arg(query);

    qDebug() << "YouTube search:" << query;

    m_process.start("/usr/local/bin/yt-dlp", args);
}

void YoutubeProvider::onSearchFinished(
    int exitCode,
    QProcess::ExitStatus status)
{
    if (status != QProcess::NormalExit || exitCode != 0)
    {
        emit errorOccurred(
            QString::fromUtf8(
                m_process.readAllStandardError()));

        return;
    }

    QByteArray json =
        m_process.readAllStandardOutput();

    QJsonParseError err;

    QJsonDocument doc =
        QJsonDocument::fromJson(json, &err);

    if (err.error != QJsonParseError::NoError)
    {
        emit errorOccurred(
            "YouTube search JSON error: " +
            err.errorString());

        return;
    }

    QJsonObject obj = doc.object();

    m_track.title =
        obj["title"].toString();

    m_track.uploader =
        obj["uploader"].toString();

    m_track.duration =
        obj["duration"].toInteger();

    m_track.thumbnailUrl =
        obj["thumbnail"].toString();

    m_track.originalUrl =
        obj["webpage_url"].toString();

    m_track.live =
        obj["is_live"].toBool();

    if (m_track.originalUrl.isEmpty())
    {
        emit errorOccurred(
            "YouTube search returned no URL.");

        return;
    }

    qDebug() << "YouTube result:";
    qDebug() << "Title:" << m_track.title;
    qDebug() << "URL:" << m_track.originalUrl;

    requestStreamUrl();
}

void YoutubeProvider::requestStreamUrl()
{
    disconnect(&m_process, nullptr, this, nullptr);

    connect(&m_process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            &YoutubeProvider::onGetUrlFinished);

    QStringList args;

    args
        << "--no-warnings"
        << "--extractor-args"
        << "youtube:player_client=web_embedded"
        << "-f"
        << "bestaudio/best"
        << "--get-url"
        << m_track.originalUrl;

    qDebug() << "Resolving YouTube audio URL...";

    m_process.start("/usr/local/bin/yt-dlp", args);
}

void YoutubeProvider::onGetUrlFinished(
    int exitCode,
    QProcess::ExitStatus status)
{
    if (status != QProcess::NormalExit || exitCode != 0)
    {
        emit errorOccurred(
            QString::fromUtf8(
                m_process.readAllStandardError()));

        return;
    }

    m_track.streamUrl =
        QString::fromUtf8(
            m_process.readAllStandardOutput()).trimmed();

    if (m_track.streamUrl.isEmpty())
    {
        emit errorOccurred(
            "yt-dlp returned an empty stream URL.");

        return;
    }

    if (!m_track.isValid())
    {
        emit errorOccurred(
            "Unable to obtain valid YouTube track.");

        return;
    }

    qDebug() << "YouTube stream URL obtained.";

    emit trackReady(m_track);
}
