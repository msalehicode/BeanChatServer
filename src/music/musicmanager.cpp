#include "musicmanager.h"

#include "../models/user.h"
#include "musicbot.h"
#include "radioresolver.h"

MusicManager::MusicManager(Server *server,
                           QObject *parent)
    : QObject(parent),
    m_server(server)
{
    m_bot = new MusicBot(server, this);

    m_player = new AudioPlayer(this);

    m_youtube = new YoutubeProvider(this);

    connect(m_player,
            &AudioPlayer::pcmFrameReady,
            m_bot,
            &MusicBot::sendPcmFrame);

    connect(m_player,
            &AudioPlayer::finished,
            this,
            &MusicManager::playNext);

    connect(m_youtube,
            &YoutubeProvider::trackReady,
            this,
            [this](const Track &track)
            {
                m_queue.enqueue(track);

                qDebug() << "========== TRACK READY ==========";
                qDebug() << "Title:" << track.title;
                qDebug() << "Uploader:" << track.uploader;
                qDebug() << "Duration:" << track.duration;
                qDebug() << "Original:" << track.originalUrl;
                qDebug() << "Stream URL exists:"
                         << !track.streamUrl.isEmpty();


                if (!m_playing)
                    playNext();
            });


    connect(m_youtube,
            &YoutubeProvider::errorOccurred,
            this,
            [](const QString &error)
            {
                qWarning() << "[Youtube]" << error;
            });

    connect(m_player,
            &AudioPlayer::errorOccurred,
            this,
            [](const QString &error)
            {
                qWarning() << "[AudioPlayer]" << error;
            });
}

void MusicManager::initialize()
{
    m_bot->initialize();

    // Later:
    //
    // m_providers.append(new YoutubeProvider(this));
    // m_providers.append(new PlaylistProvider(this));
}

bool MusicManager::play(UserModel *sender,
                        const QString &query)
{
    qDebug() << "musicmanager play called";
    if (!sender)
        return false;

    if (!sender->currentChannel)
        return false;

    if (!m_bot->currentChannel())
    {
        qDebug() << "bot channel is not as the sender, try to join user's channel.";
        if (!m_bot->joinChannel(sender->currentChannel))
        {
            qWarning() << "MusicBot failed to join user's channel.";
            return false;
        }
    }


    //resolve sent radio url
    const QString streamUrl = RadioResolver::resolve(query);
    if (!streamUrl.isEmpty())
    {
        qDebug() << "[MusicManager] Radio stream:" << streamUrl;

        Track track;
        track.title = "Internet Radio";
        track.originalUrl = query;
        track.streamUrl = streamUrl;
        track.live = true;

        m_queue.enqueue(track);

        if (!m_playing)
            playNext();

        return true;
    }
    else
    {
        qWarning() << "Could not resolve radio URL:" << query << "lets search youtube";
        m_youtube->search(query);
    }


    return true;
}
void MusicManager::playNext()
{
    if (m_queue.isEmpty())
    {
        m_playing = false;
        m_hasCurrentTrack = false;

        return;
    }

    m_currentTrack = m_queue.dequeue();

    m_hasCurrentTrack = true;
    m_playing = true;
    m_paused = false;

    m_player->play(m_currentTrack);
}

void MusicManager::stop()
{
    m_player->stop();

    m_bot->leaveChannel(); // optional

    m_playing = false;
    m_paused = false;
    m_hasCurrentTrack = false;
}

void MusicManager::skip()
{
    m_player->stop();

    playNext();
}

void MusicManager::pause()
{
    if (!m_playing)
        return;

    m_player->pause();

    m_paused = true;
}

void MusicManager::resume()
{
    if (!m_playing)
        return;

    m_player->resume();

    m_paused = false;
}

void MusicManager::clearQueue()
{
    m_queue.clear();
}

bool MusicManager::isPlaying() const
{
    return m_playing;
}

bool MusicManager::isPaused() const
{
    return m_paused;
}

const QQueue<Track> &MusicManager::queue() const
{
    return m_queue;
}

const Track *MusicManager::currentTrack() const
{
    return m_hasCurrentTrack ? &m_currentTrack : nullptr;
}

MusicBot *MusicManager::bot() const
{
    return m_bot;
}
