#pragma once

#include <QObject>
#include <QQueue>

#include "track.h"

#include "musicbot.h"
#include "audioplayer.h"
#include "providers/youtubeprovider.h"

class Server;
class UserModel;
class MusicBot;
class TrackProvider;

class MusicManager : public QObject
{
    Q_OBJECT

public:
    explicit MusicManager(Server *server,
                          QObject *parent = nullptr);

    void initialize();

    bool play(UserModel *sender,
              const QString &query);

    void stop();

    void skip();

    void pause();

    void resume();

    void clearQueue();

    bool isPlaying() const;

    bool isPaused() const;

    const QQueue<Track> &queue() const;

    const Track *currentTrack() const;

    MusicBot *bot() const;

private:
    void playNext();

    Server *m_server = nullptr;

    MusicBot *m_bot = nullptr;

    AudioPlayer *m_player = nullptr;

    YoutubeProvider *m_youtube = nullptr;

    QList<TrackProvider *> m_providers;

    QQueue<Track> m_queue;

    Track m_currentTrack;

    bool m_hasCurrentTrack = false;
    bool m_playing = false;
    bool m_paused = false;
};
