#include "opuscodec.h"

// #include "logging/loggingcategories.h"

OpusCodec::OpusCodec(QObject *parent)
    : QObject(parent)
{
}

OpusCodec::~OpusCodec()
{
    shutdown();
}

bool OpusCodec::initialize(
    int sampleRate,
    int channels,
    int bitrate)
{
    shutdown();

    m_sampleRate = sampleRate;
    m_channels = channels;
    m_bitrate = bitrate;

    switch (sampleRate)
    {
    case 8000:
        m_frameSize = 160;
        break;

    case 12000:
        m_frameSize = 240;
        break;

    case 16000:
        m_frameSize = 320;
        break;

    case 24000:
        m_frameSize = 480;
        break;

    case 48000:
        m_frameSize = 960;
        break;

    default:
        qWarning() <<  "Unsupported sample rate:" << sampleRate;
        return false;
    }

    int error = OPUS_OK;

    m_encoder = opus_encoder_create(
        m_sampleRate,
        m_channels,
        OPUS_APPLICATION_VOIP,
        &error);

    if (error != OPUS_OK || !m_encoder)
    {
        qCritical()  << "Failed to create Opus encoder:" << opus_strerror(error);

        shutdown();
        return false;
    }

    m_decoder = opus_decoder_create(
        m_sampleRate,
        m_channels,
        &error);

    if (error != OPUS_OK || !m_decoder)
    {
        qWarning() << "Failed to create Opus decoder:" << opus_strerror(error);

        shutdown();
        return false;
    }

    // -------------------------------
    // Encoder configuration
    // -------------------------------

    opus_encoder_ctl(
        m_encoder,
        OPUS_SET_BITRATE(m_bitrate));

    opus_encoder_ctl(
        m_encoder,
        OPUS_SET_COMPLEXITY(10));

    opus_encoder_ctl(
        m_encoder,
        OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));

    opus_encoder_ctl(
        m_encoder,
        OPUS_SET_INBAND_FEC(1));

    opus_encoder_ctl(
        m_encoder,
        OPUS_SET_DTX(1));

    opus_encoder_ctl(
        m_encoder,
        OPUS_SET_VBR(1));

    opus_encoder_ctl(
        m_encoder,
        OPUS_SET_VBR_CONSTRAINT(0));

    opus_encoder_ctl(
        m_encoder,
        OPUS_SET_PACKET_LOSS_PERC(10));

    qInfo()
        << "Opus initialized"
        << "SampleRate:" << m_sampleRate
        << "Channels:" << m_channels
        << "FrameSize:" << m_frameSize
        << "Bitrate:" << m_bitrate;

    return true;
}

void OpusCodec::shutdown()
{
    if (m_encoder)
    {
        opus_encoder_destroy(m_encoder);
        m_encoder = nullptr;
        qInfo() << "Shutting down encoder";
    }

    if (m_decoder)
    {
        opus_decoder_destroy(m_decoder);
        m_decoder = nullptr;
        qInfo() << "Shutting down decoder";
    }
}

bool OpusCodec::isInitialized() const
{
    return m_encoder != nullptr &&
           m_decoder != nullptr;
}

int OpusCodec::sampleRate() const
{
    return m_sampleRate;
}

int OpusCodec::channels() const
{
    return m_channels;
}

int OpusCodec::frameSize() const
{
    return m_frameSize;
}

int OpusCodec::bitrate() const
{
    return m_bitrate;
}

void OpusCodec::setBitrate(int bitrate)
{
    m_bitrate = bitrate;

    if (m_encoder)
    {
        opus_encoder_ctl(
            m_encoder,
            OPUS_SET_BITRATE(m_bitrate));
    }
}





QByteArray OpusCodec::encode(const QByteArray &pcm)
{
    if (!m_encoder)
        return {};

    const int expectedBytes =
        m_frameSize *
        m_channels *
        sizeof(opus_int16);

    if (pcm.size() != expectedBytes)
    {
        qWarning()
            << "Opus encode expects"
            << expectedBytes
            << "bytes but got"
            << pcm.size();

        return {};
    }

    constexpr int MaxPacketSize = 4000;

    QByteArray encoded;
    encoded.resize(MaxPacketSize);

    const opus_int16 *input =
        reinterpret_cast<const opus_int16*>(pcm.constData());

    int bytes = opus_encode(
        m_encoder,
        input,
        m_frameSize,
        reinterpret_cast<unsigned char*>(encoded.data()),
        MaxPacketSize);

    if (bytes < 0)
    {
        qWarning() << "opus_encode failed:" << opus_strerror(bytes);

        return {};
    }

    encoded.resize(bytes);

    return encoded;
}

QByteArray OpusCodec::decode(const QByteArray &opusData)
{
    if (!m_decoder)
        return {};

    QByteArray pcm;
    pcm.resize(
        m_frameSize *
        m_channels *
        sizeof(opus_int16));

    opus_int16 *output =
        reinterpret_cast<opus_int16*>(pcm.data());

    int samples = opus_decode(
        m_decoder,
        reinterpret_cast<const unsigned char*>(opusData.constData()),
        opusData.size(),
        output,
        m_frameSize,
        0);

    if (samples < 0)
    {
        qWarning() << "opus_decode failed:" << opus_strerror(samples);

        return {};
    }

    pcm.resize(
        samples *
        m_channels *
        sizeof(opus_int16));

    return pcm;
}
