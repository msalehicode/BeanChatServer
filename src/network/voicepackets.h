#pragma once

#include <QByteArray>
#include <QDataStream>

struct UdpRegisterPacket
{
    quint64 userId;
};

inline QDataStream&
operator<<(QDataStream& out,
           const UdpRegisterPacket& p)
{
    out << p.userId;
    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           UdpRegisterPacket& p)
{
    in >> p.userId;
    return in;
}

struct VoicePacket
{
    quint64 senderId=0;

    quint32 sequence=0;

    QByteArray audioData;
};

inline QDataStream&
operator<<(QDataStream& out,
           const VoicePacket& p)
{
    out << p.senderId
        << p.sequence
        << p.audioData;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           VoicePacket& p)
{
    in >> p.senderId
        >> p.sequence
        >> p.audioData;

    return in;
}
