#pragma once

#include <QByteArray>
#include <QDataStream>

namespace PacketHelpers
{

template<typename T>
QByteArray pack(
    const T& value)
{
    QByteArray buffer;

    QDataStream out(
        &buffer,
        QIODevice::WriteOnly);

    out << value;

    return buffer;
}

template<typename T>
T unpack(
    const QByteArray& data)
{
    T value;

    QDataStream in(data);

    in >> value;

    return value;
}

}
