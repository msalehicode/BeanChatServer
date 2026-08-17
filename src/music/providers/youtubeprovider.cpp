#include "youtubeprovider.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

YoutubeProvider::YoutubeProvider(QObject *parent)
    : QObject(parent)
{
}

void YoutubeProvider::search(const QString &query)
{
    Q_UNUSED(query);

    m_track = Track();

    m_track.originalUrl =
        "https://www.youtube.com/watch?v=YQHsXMglC9A";

    requestStreamUrl();
}

// void YoutubeProvider::search(const QString &query)
// {
//     if (m_process.state() != QProcess::NotRunning)
//         m_process.kill();

//     m_query = query;
//     m_track = Track();

//     disconnect(&m_process, nullptr, this, nullptr);

//     connect(&m_process,
//             qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
//             this,
//             &YoutubeProvider::onSearchFinished);


//     QStringList args;




//     // args
//     //     << "--proxy"
//     //     << "http://169.254.1.1:8080"
//     //     << "--cookies"
//     //     << "/home/mrx/.config/beanchat/youtube_cookies.txt"
//     //     << "--js-runtimes"
//     //     << "node"
//     //     << "--no-warnings"
//     //     << "--dump-single-json"
//     //     << "--no-playlist"
//     //     << QString("ytsearch1:%1").arg(query);

//     args
//         << "--proxy"
//         << "http://169.254.1.1:8080"
//         << "--cookies-from-browser"
//         << "chromium:/home/mrx/snap/chromium/common/chromium"
//         << "--js-runtimes"
//         << "node"
//         << "--no-warnings"
//         << "--dump-single-json"
//         << "--no-playlist"
//         << QString("ytsearch1:%1").arg(query);
//     //apply proxy
//     // QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
//     // env.insert("http_proxy", "http://169.254.1.1:8080");
//     // env.insert("https_proxy", "http://169.254.1.1:8080");
//     // m_process.setProcessEnvironment(env);


//     // m_process.start("yt-dlp", args);
//     m_process.start("/usr/local/bin/yt-dlp", args);
// }




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

    auto doc =
        QJsonDocument::fromJson(json, &err);

    if (err.error != QJsonParseError::NoError)
    {
        emit errorOccurred(err.errorString());

        return;
    }

    auto obj =
        doc.object();

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

    // args
    //     << "--proxy"
    //     << "http://169.254.1.1:8080"
    //     << "--cookies-from-browser"
    //     << "chromium:/home/mrx/snap/chromium/common/chromium"
    //     << "--js-runtimes"
    //     << "node"
    //     << "--no-warnings"
    //     << "-f"
    //     << "bestaudio/best"
    //     << "--get-url"
    //     << m_track.originalUrl;

    args
        << "--proxy"
        << "http://169.254.1.1:8080"
        << "--cookies-from-browser"
        << "chromium:/home/mrx/snap/chromium/common/chromium"
        << "--js-runtimes"
        << "node"
        << "--no-warnings"
        << "-f"
        << "bestaudio/best"
        << "--get-url"
        << m_track.originalUrl;

    m_process.start("/usr/local/bin/yt-dlp", args);
}
// void YoutubeProvider::requestStreamUrl()
// {
//     disconnect(&m_process, nullptr, this, nullptr);

//     connect(&m_process,
//             qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
//             this,
//             &YoutubeProvider::onGetUrlFinished);

//     QStringList args;

//     // args
//     //     << "--proxy"
//     //     << "http://169.254.1.1:8080"
//     //     << "--cookies"
//     //     << "/home/mrx/.config/beanchat/youtube_cookies.txt"
//     //     << "--js-runtimes"
//     //     << "node"
//     //     << "--no-warnings"
//     //     << "-f"
//     //     << "ba"
//     //     << "--get-url"
//     //     << m_track.originalUrl;

//     args
//         << "--proxy"
//         << "http://169.254.1.1:8080"
//         << "--cookies-from-browser"
//         << "firefox"
//         << "--js-runtimes"
//         << "node"
//         << "--no-warnings"
//         << "-f"
//         << "ba"
//         << "--get-url"
//         << m_track.originalUrl;

//     // QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
//     // env.insert("http_proxy", "http://169.254.1.1:8080");
//     // env.insert("https_proxy", "http://169.254.1.1:8080");
//     // m_process.setProcessEnvironment(env);

//     // m_process.start("yt-dlp", args);
//     m_process.start("/usr/local/bin/yt-dlp", args);
// }


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

    if (!m_track.isValid())
    {
        emit errorOccurred(
            "Unable to obtain stream url.");

        return;
    }

    emit trackReady(m_track);
}
