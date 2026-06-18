#include "udpserver.h"

#include "../server/server.h"
#include "../models/user.h"
#include "../models/channel.h"

#include "voicepackets.h"

#include <QNetworkDatagram>
#include <QDebug>

UdpServer::UdpServer(
    Server* server,
    QObject* parent)
    :
    QObject(parent),
    m_server(server)
{
    connect(
        &m_socket,
        &QUdpSocket::readyRead,
        this,
        &UdpServer::onReadyRead);
}

bool UdpServer::start(
    quint16 port)
{
    bool ok =
        m_socket.bind(
            QHostAddress::Any,
            port);

    qDebug()
        << "udpvoice Listening on"
        << port;

    return ok;
}

User* UdpServer::findUser(
    quint64 userId)
{
    for(auto user :
         m_server->users())
    {
        if(user->id == userId)
            return user;
    }

    return nullptr;
}

void UdpServer::onReadyRead()
{
    while(m_socket.hasPendingDatagrams())
    {
        auto datagram =
            m_socket.receiveDatagram();

        QByteArray raw =
            datagram.data();

        QDataStream stream(raw);

        quint16 packetType;

        stream >> packetType;

        qDebug()
            << "UDP TYPE:"
            << packetType;

        switch(packetType)
        {
        case 100:
            processRegister(
                datagram,
                stream);
            break;

        case 101:
            processVoice(
                datagram,
                stream);
            break;

        default:
            qDebug()
                << "Unknown UDP packet:"
                << packetType;
            break;
        }
    }
}

void UdpServer::processRegister(
    const QNetworkDatagram& datagram,
    QDataStream& stream)
{
    UdpRegisterPacket packet;

    stream >> packet;

    auto user =
        findUser(
            packet.userId);

    if(!user)
    {
        qDebug()
        << "UDP register failed";

        return;
    }

    user->udpAddress =
        datagram.senderAddress();

    user->udpPort =
        datagram.senderPort();

    user->udpRegistered =
        true;

    qDebug()
        << user->username
        << "UDP registered";
}

void UdpServer::processVoice(
    const QNetworkDatagram&,
    QDataStream& stream)
{
    VoicePacket packet;

    stream >> packet;

    qDebug()
        << "VOICE:"
        << packet.senderId
        << packet.sequence
        << packet.audioData.size();

    auto sender =
        findUser(
            packet.senderId);

    if(!sender)
    {
        qDebug()
        << "Voice sender not found";

        return;
    }

    if(!sender->currentChannel)
    {
        qDebug()
        << "Sender has no channel";

        return;
    }

    auto channel =
        sender->currentChannel;

    QByteArray outData;

    QDataStream out(
        &outData,
        QIODevice::WriteOnly);

    out << quint16(101);
    out << packet;

    for(auto user :
         channel->users)
    {
        if(user == sender)
            continue;

        if(!user->udpRegistered)
            continue;

        qint64 bytes =
            m_socket.writeDatagram(
                outData,
                user->udpAddress,
                user->udpPort);

        qDebug()
            << "Forwarded"
            << bytes
            << "bytes to"
            << user->username;
    }
}
