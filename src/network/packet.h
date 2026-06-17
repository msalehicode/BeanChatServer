#pragma once

#include <QByteArray>
#include <QDataStream>

#include "packets.h"

struct Packet
{
    PacketType type =
        PacketType::Invalid;

    QByteArray payload;

    QByteArray serialize() const
    {
        QByteArray buffer;

        QDataStream out(
            &buffer,
            QIODevice::WriteOnly);

        out << static_cast<quint16>(
            type);

        out << static_cast<quint32>(
            payload.size());

        out.writeRawData(
            payload.constData(),
            payload.size());

        return buffer;
    }

    static Packet deserialize(
        const QByteArray& data)
    {
        Packet packet;

        QDataStream in(data);

        quint16 type;
        quint32 size;

        in >> type;
        in >> size;

        packet.type =
            static_cast<PacketType>(
                type);

        packet.payload.resize(
            size);

        in.readRawData(
            packet.payload.data(),
            size);

        return packet;
    }
};
