#pragma once

#include <QObject>
#include <QByteArray>
#include <QDebug>

#include <opus/opus.h>

class OpusCodec : public QObject
{
    Q_OBJECT

public:
    explicit OpusCodec(QObject *parent = nullptr);
    ~OpusCodec();

    bool initialize(
        int sampleRate = 48000,
        int channels = 1,
        int bitrate = 32000);

    void shutdown();

    bool isInitialized() const;

    QByteArray encode(const QByteArray &pcm);

    QByteArray decode(const QByteArray &opusData);

    int sampleRate() const;
    int channels() const;
    int frameSize() const;
    int bitrate() const;

    void setBitrate(int bitrate);

private:
    OpusEncoder *m_encoder = nullptr;
    OpusDecoder *m_decoder = nullptr;

    int m_sampleRate = 48000;
    int m_channels = 1;

    int m_frameSize = 960;

    int m_bitrate = 32000;
};
