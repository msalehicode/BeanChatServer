#include "audioplayer.h"

#include <QDebug>

AudioPlayer::AudioPlayer(QObject *parent)
    : QObject(parent)
{
    connect(&m_ffmpeg,
            &QProcess::readyReadStandardOutput,
            this,
            &AudioPlayer::onReadyRead);

    connect(&m_ffmpeg,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            &AudioPlayer::onFinished);


    m_frameTimer.setTimerType(Qt::PreciseTimer);
    m_frameTimer.setInterval(20);

    connect(
        &m_frameTimer,
        &QTimer::timeout,
        this,
        &AudioPlayer::processBuffer);
}

AudioPlayer::~AudioPlayer()
{
    stop();
}

bool AudioPlayer::play(const Track &track)
{
    stop();

    m_currentTrack = track;
    m_buffer.clear();

    QString program = "ffmpeg";

    QStringList args;

    args << "-loglevel" << "error";
    args << "-i" << track.streamUrl;

    args << "-vn";

    args << "-ac" << "1";

    args << "-ar" << "48000";

    args << "-f" << "s16le";

    args << "pipe:1";

    m_ffmpeg.start(program, args);

    if (!m_ffmpeg.waitForStarted())
    {
        emit errorOccurred("Unable to start FFmpeg.");

        return false;
    }

    m_playing = true;
    m_paused = false;

    m_frameTimer.start();


    return true;
}

void AudioPlayer::stop()
{
    if (m_ffmpeg.state() != QProcess::NotRunning)
    {
        m_ffmpeg.kill();
        m_ffmpeg.waitForFinished();
    }

    m_frameTimer.stop();

    m_buffer.clear();

    m_playing = false;
    m_paused = false;

}

void AudioPlayer::pause()
{
    if (!m_playing)
        return;

    m_paused = true;
}

void AudioPlayer::resume()
{
    if (!m_playing)
        return;

    m_paused = false;
}

bool AudioPlayer::isPlaying() const
{
    return m_playing;
}

bool AudioPlayer::isPaused() const
{
    return m_paused;
}


void AudioPlayer::onReadyRead()
{
    QByteArray data = m_ffmpeg.readAllStandardOutput();

    if (m_paused)
        return;

    m_buffer.append(data);

    // qDebug() << "[AudioPlayer]" << "received:" << data.size() << "buffer:" << m_buffer.size();
}

void AudioPlayer::processBuffer()
{
    if (m_buffer.size() < FrameBytes)
        return;

    QByteArray pcm = m_buffer.left(FrameBytes);
    m_buffer.remove(0, FrameBytes);

    emit pcmFrameReady(pcm);
}

void AudioPlayer::onFinished(int,
                             QProcess::ExitStatus)
{
    m_playing = false;
    m_paused = false;

    emit finished();
}
