#pragma once

#include <QString>
#include <QUrl>

class RadioResolver
{
public:
    static QString resolve(const QString &query);

private:
    static QString resolvePlaylistGenerator(const QUrl &url);
    static QString resolveInternetRadioPlayer(const QUrl &url);
    static QString resolvePlaylist(const QUrl &url);
    static QString resolveWebPage(const QUrl &url);

    static QString firstUrlFromPlaylist(const QByteArray &data);
};
