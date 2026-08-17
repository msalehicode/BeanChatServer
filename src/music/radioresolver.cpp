#include "radioresolver.h"

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
QString RadioResolver::resolve(const QString &query)
{
    const QString input = query.trimmed();

    if (input.isEmpty())
        return {};

    QUrl url(input);

    if (!url.isValid() || url.scheme().isEmpty())
        return {};

    qDebug() << "[RadioResolver] Resolving:" << input;

    /*
     * internet-radio.com player URL
     *
     * ?server=uk3.internet-radio.com:8405
     * ?server=http://149.56.234.138:8025
     */
    if (url.host().compare(
            "player.internet-radio.com",
            Qt::CaseInsensitive) == 0)
    {
        return resolveInternetRadioPlayer(url);
    }

    /*
     * internet-radio.com playlist generator
     *
     * ?u=https://example.com/radio.m3u&t=.m3u
     */
    if (url.host().compare(
            "www.internet-radio.com",
            Qt::CaseInsensitive) == 0 &&
        url.path().contains("/servers/tools/playlistgenerator"))
    {
        return resolvePlaylistGenerator(url);
    }

    const QString path = url.path().toLower();

    /*
     * Direct playlist.
     */
    if (path.endsWith(".pls") ||
        path.endsWith(".m3u") ||
        path.endsWith(".m3u8"))
    {
        const QString result = resolvePlaylist(url);

        if (!result.isEmpty())
            return result;
    }

    /*
     * Otherwise try it as a normal radio/web URL.
     */
    const QString result = resolveWebPage(url);

    if (!result.isEmpty())
        return result;

    /*
     * Assume the user already gave us a direct stream.
     */
    return input;
}

QString RadioResolver::resolvePlaylistGenerator(const QUrl &url)
{
    QUrlQuery query(url);

    QString playlistUrl =
        query.queryItemValue("u").trimmed();

    if (playlistUrl.isEmpty())
        return {};

    playlistUrl = QUrl::fromPercentEncoding(
        playlistUrl.toUtf8()
        );

    qDebug() << "[RadioResolver] Playlist generator target:"
             << playlistUrl;

    return resolve(playlistUrl);
}

QString RadioResolver::resolveInternetRadioPlayer(const QUrl &url)
{
    QUrlQuery query(url);

    QString server = query.queryItemValue("server").trimmed();

    if (server.isEmpty())
        return {};

    /*
     * HTML/web URLs may have encoded values.
     */
    server = QUrl::fromPercentEncoding(
        server.toUtf8()
        );

    qDebug() << "[RadioResolver] Server parameter:"
             << server;

    /*
     * Already contains a scheme:
     *
     * http://149.56.234.138:8025
     * https://example.com:8000/stream
     */
    if (server.startsWith("http://",
                          Qt::CaseInsensitive) ||
        server.startsWith("https://",
                          Qt::CaseInsensitive))
    {
        qDebug() << "[RadioResolver] Player URL resolved to:"
                 << server;

        return server;
    }

    /*
     * Otherwise:
     *
     * uk3.internet-radio.com:8405
     *
     * -> http://uk3.internet-radio.com:8405/
     */
    const QString result =
        "http://" + server + "/";

    qDebug() << "[RadioResolver] Player URL resolved to:"
             << result;

    return result;
}

QString RadioResolver::resolvePlaylist(const QUrl &url)
{
    QNetworkAccessManager manager;
    QNetworkRequest request(url);

    request.setHeader(
        QNetworkRequest::UserAgentHeader,
        "BeanChat/1.0"
        );

    QNetworkReply *reply = manager.get(request);

    QEventLoop loop;

    QObject::connect(
        reply,
        &QNetworkReply::finished,
        &loop,
        &QEventLoop::quit
        );

    loop.exec();

    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning() << "[RadioResolver] Playlist download failed:"
                   << reply->errorString();

        reply->deleteLater();
        return {};
    }

    const QByteArray data = reply->readAll();

    reply->deleteLater();

    return firstUrlFromPlaylist(data);
}


QString RadioResolver::firstUrlFromPlaylist(const QByteArray &data)
{
    const QString text = QString::fromUtf8(data);

    const QStringList lines = text.split(
        QRegularExpression("[\\r\\n]+"),
        Qt::SkipEmptyParts
        );

    for (const QString &line : lines)
    {
        const QString trimmed = line.trimmed();

        if (trimmed.startsWith("#"))
            continue;

        /*
         * PLS:
         *
         * File1=http://example.com:8000/stream
         */
        if (trimmed.startsWith("File",
                               Qt::CaseInsensitive))
        {
            const int equals = trimmed.indexOf('=');

            if (equals >= 0)
            {
                const QString value =
                    trimmed.mid(equals + 1).trimmed();

                if (value.startsWith("http://") ||
                    value.startsWith("https://"))
                {
                    return value;
                }
            }
        }

        /*
         * M3U:
         *
         * http://example.com:8000/stream
         */
        if (trimmed.startsWith("http://") ||
            trimmed.startsWith("https://"))
        {
            return trimmed;
        }
    }

    return {};
}

QString RadioResolver::resolveWebPage(const QUrl &url)
{
    QNetworkAccessManager manager;
    QNetworkRequest request(url);

    request.setHeader(
        QNetworkRequest::UserAgentHeader,
        "Mozilla/5.0 BeanChat/1.0"
        );

    QNetworkReply *reply = manager.get(request);

    QEventLoop loop;

    QObject::connect(
        reply,
        &QNetworkReply::finished,
        &loop,
        &QEventLoop::quit
        );

    loop.exec();

    if (reply->error() != QNetworkReply::NoError)
    {
        reply->deleteLater();
        return {};
    }

    const QByteArray data = reply->readAll();

    reply->deleteLater();

    const QString html = QString::fromUtf8(data);

    /*
     * Look for:
     *
     * <audio src="...">
     * <source src="...">
     */

    QRegularExpression re(
        R"(<(?:audio|source)[^>]+src\s*=\s*["']([^"']+)["'])",
        QRegularExpression::CaseInsensitiveOption
        );

    const QRegularExpressionMatch match = re.match(html);

    if (!match.hasMatch())
        return {};

    const QString found = match.captured(1);

    QUrl streamUrl = url.resolved(QUrl(found));

    if (!streamUrl.isValid())
        return {};

    qDebug() << "[RadioResolver] Found stream:"
             << streamUrl.toString();

    return streamUrl.toString();
}
