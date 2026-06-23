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


    //setup ping timer
    connect(
        &m_pingTimer,
        &QTimer::timeout,
        this,
        &UdpServer::sendPings);
    m_pingTimer.start(2000);


    //setup packetlosses timer
    connect(
        &m_packetLossTimer,
        &QTimer::timeout,
        this,
        &UdpServer::updatePacketLoss);
    m_packetLossTimer.start(5000);
}

bool UdpServer::start(
    quint16 port)
{
    bool ok =
        m_socket.bind(
            QHostAddress::Any,
            port);

    qDebug()
        << "udpVoice and udpVideo Listening on"
        << port;

    return ok;
}

void UdpServer::sendPings()
{
    const qint64 now =
        QDateTime::currentMSecsSinceEpoch();

    for(auto user : m_server->users())
    {
        if(!user->udpRegistered)
            continue;

        PingPacket packet;

        packet.sequence = ++user->nextPingSequence;
        packet.lastPing = user->ping;
        packet.voicePacketLoss = user->voicePacketLossStats.packetLoss;
        packet.videoPacketLoss = user->videoPacketLossStats.packetLoss;

        user->pendingPings[packet.sequence] = now;

        user->pingsSent++;

        QByteArray data;

        QDataStream out(
            &data,
            QIODevice::WriteOnly);

        out << quint16(103); //103 is ping sign
        out << packet;

        m_socket.writeDatagram(
            data,
            user->udpAddress,
            user->udpPort);


        //new to cleanup!
        auto it =
            user->pendingPings.begin();

        while(it != user->pendingPings.end())
        {
            if(now - it.value() > 10000)
            {
                it =
                    user->pendingPings.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
}

void UdpServer::processPong(
    const QNetworkDatagram& datagram,
    QDataStream& stream)
{
    PingPacket packet;

    stream >> packet;

    UserModel* user = nullptr;

    for(auto u : m_server->users())
    {
        if(!u->udpRegistered)
            continue;

        if(u->udpAddress ==
                datagram.senderAddress()
            &&
            u->udpPort ==
                datagram.senderPort())
        {
            user = u;
            break;
        }
    }

    if(!user)
        return;

    if(!user->pendingPings.contains(
            packet.sequence))
    {
        return;
    }

    const qint64 sentTime =
        user->pendingPings.take(
            packet.sequence);

    const qint64 now =
        QDateTime::currentMSecsSinceEpoch();

    user->ping =
        int(now - sentTime);

    user->pongsReceived++;

    // qDebug()
    //     << user->username
    //     << "Ping:"
    //     << user->ping
    //     << "ms";
}

void UdpServer::updatePacketLoss()
{
    for(UserModel* user : m_server->users())
    {
        if(!user)
            continue;

        //voice
        calculateUpdatePacketLoss(user->voicePacketLossStats);

        //video
        calculateUpdatePacketLoss(user->videoPacketLossStats);
    }
}

void UdpServer::calculateUpdatePacketLoss(PacketLossStats& stats)
{
    quint64 expected =
        stats.windowReceived +
        stats.windowLost;

    if(expected > 0)
    {
        stats.packetLoss =
            100.0f *
            float(stats.windowLost)
            / float(expected);
    }
    else
    {
        stats.packetLoss = 0.0f;
    }

    stats.windowReceived = 0;
    stats.windowLost = 0;
}

void UdpServer::calculatePacketLoss(quint32 packetSequence, PacketLossStats &stats)
{
    if(stats.windowReceived == 0)
        stats.highestSequence = packetSequence;
    else
    {
        if(packetSequence > stats.highestSequence + 1)
            stats.windowLost += packetSequence- stats.highestSequence -1;

        if(packetSequence > stats.highestSequence)
            stats.highestSequence = packetSequence;
    }
    stats.windowReceived++;
}

UserModel* UdpServer::findUser(
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

        // qDebug()
            // << "UDP TYPE:"
            // << packetType;

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

        case 102:
            processVideo(
                datagram,
                stream);
            break;

        // case 103: //reserved ping request sends to client.

        case 104:
            processPong(
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
        qDebug() << "UDP register failed";

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
        << "UDP registered" << "user addres= " << user->udpAddress << " port:" << user->udpPort;
}

void UdpServer::processVoice(
    const QNetworkDatagram& datagram,
    QDataStream& stream)
{
    VoicePacket packet;

    stream >> packet;

    // qDebug()
    //     << "VOICE:"
    //     << packet.senderId
    //     << packet.sequence
    //     << packet.audioData.size();

    auto sender = findUser(packet.senderId);

    if(!sender)
    {
        // qDebug() << "Voice sender not found";
        return;
    }

    if(!sender->currentChannel)
    {
        // qDebug() << "Sender has no channel";
        return;
    }


    //check if senderId's ip:port matches with that user?
    if(datagram.senderAddress() != sender->udpAddress || datagram.senderPort() != sender->udpPort)
    {
        qDebug() << "userId doesnt match with senders ip:port";
        return;
    }


    auto channel =
        sender->currentChannel;

    if(!channel)
        return;


    //calculate packetloss
    calculatePacketLoss(packet.sequence, sender->voicePacketLossStats);


    QByteArray outData;

    QDataStream out(
        &outData,
        QIODevice::WriteOnly);

    out << quint16(101);
    out << packet;

    for(auto user : channel->users)
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

        // qDebug() << "Forwarded" << bytes << "bytes to" << user->username;
    }
}

void UdpServer::processVideo(const QNetworkDatagram &datagram, QDataStream &stream)
{
    VideoPacket packet;

    stream >> packet;

    // qDebug()
    //     << "Video:"
    //     << packet.senderId
    //     << packet.sequence
    //     << packet.videoData.size();

    auto sender =
        findUser(
            packet.senderId);

    if(!sender)
    {
        qDebug() << "video sender not found";
        return;
    }

    if(!sender->currentChannel)
    {
        qDebug() << "Sender has no channel";
        return;
    }


    //check if senderId's ip:port matches with that user?
    if(datagram.senderAddress() != sender->udpAddress || datagram.senderPort() != sender->udpPort)
    {
        qDebug() << "userId doesnt match with senders ip:port";
        return;
    }


    auto channel = sender->currentChannel;

    if(!channel)
        return;


    calculatePacketLoss(packet.sequence, sender->videoPacketLossStats);



    QByteArray outData;

    QDataStream out(
        &outData,
        QIODevice::WriteOnly);

    out << quint16(102);
    out << packet;

    for(auto user : channel->users)
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

        // qDebug() << "Forwarded video" << bytes << "bytes to" << user->username;
    }
}
