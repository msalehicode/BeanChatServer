#pragma once

#include <QObject>
#include <QProcess>
#include <QByteArray>
#include <QTimer>
#include "track.h"

class AudioPlayer : public QObject
{
    Q_OBJECT

public:
    explicit AudioPlayer(QObject *parent = nullptr);
    ~AudioPlayer() override;

    bool play(const Track &track);

    void stop();

    void pause();

    void resume();

    bool isPlaying() const;
    bool isPaused() const;

signals:
    void pcmFrameReady(const QByteArray &pcm);

    void finished();

    void errorOccurred(const QString &error);

private slots:
    void onReadyRead();

    void onFinished(int exitCode,
                    QProcess::ExitStatus status);

private:
    void processBuffer();

private:
    QProcess m_ffmpeg;

    QByteArray m_buffer;

    bool m_playing = false;

    bool m_paused = false;

    Track m_currentTrack;
    QTimer m_frameTimer;
    static constexpr int SampleRate = 48000;
    static constexpr int Channels = 1;
    static constexpr int SamplesPerFrame = 960;
    static constexpr int BytesPerSample = sizeof(qint16);

    static constexpr int FrameBytes =
        SamplesPerFrame *
        Channels *
        BytesPerSample;
};
