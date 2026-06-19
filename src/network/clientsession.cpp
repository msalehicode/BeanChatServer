#include "clientsession.h"

#include "../network/packethelpers.h"
#include "../network/packets.h"

#include "../server/server.h"

#include "../models/user.h"

#include <QDebug>

ClientSession::ClientSession(
    QTcpSocket* socket,
    Server* server)
    :
    m_socket(socket),
    m_server(server)
{
    connect(
        m_socket,
        &QTcpSocket::readyRead,
        this,
        &ClientSession::onReadyRead);

    connect(
        m_socket,
        &QTcpSocket::disconnected,
        this,
        &ClientSession::onDisconnected);
}

User* ClientSession::user() const
{
    return m_user;
}

void ClientSession::sendPacket(
    PacketType type,
    const QByteArray& payload)
{
    Packet packet;

    packet.type = type;
    packet.payload = payload;

    m_socket->write(
        packet.serialize());
}

void ClientSession::onReadyRead()
{
    m_buffer.append(
        m_socket->readAll());

    while(true)
    {
        if(m_buffer.size() < 6)
            return;

        QDataStream header(m_buffer);

        quint16 type;
        quint32 size;

        header >> type;
        header >> size;

        if(m_buffer.size() < (6 + size))
            return;

        QByteArray packetBytes =
            m_buffer.left(6 + size);

        m_buffer.remove(
            0,
            6 + size);

        Packet packet =
            Packet::deserialize(
                packetBytes);

        processPacket(packet);
    }
}

void ClientSession::handleLogin(const QByteArray& payload)
{
    auto req =
        PacketHelpers::unpack<LoginRequestPacket>(
            payload);

    qDebug()
        << "login user...";


    if(req.username.isEmpty() || req.identity.isEmpty())
        return;

    m_user =
        m_server->loginUser(
            req.username,
            req.identity,
            m_socket);

    // LoginResponsePacket resp;

    // resp.accepted =
    //     (m_user != nullptr);

    // resp.message =
    //     resp.accepted
    //         ? "OK"
    //         : "Rejected";

    // sendPacket(
    //     PacketType::LoginResponse,
    //     PacketHelpers::pack(resp));
}

void ClientSession::processPacket(
    const Packet& packet)
{
    qDebug()
    << "processPacket: type="
    << static_cast<int>(
           packet.type);

    switch(packet.type)
    {
    case PacketType::RequestServerState:
    {
        if(!m_user)
            break;

        sendPacket(
            PacketType::ServerState,
            m_server->buildServerState());
    }
    case PacketType::LoginRequest:
    {
        handleLogin(
            packet.payload);
    }
    break;

    case PacketType::CreateChannel:
    {
        auto req =
            PacketHelpers::unpack<CreateChannelPacket>(
                packet.payload);

        m_server->createChannel(
            req.name,
            req.password,
            req.permanentChat,
            req.temporaryChat);


        m_server->printChannelWithUsersIn();
    }
    break;

    case PacketType::JoinChannel:
    {
        if(!m_user)
            break;

        auto req =
            PacketHelpers::unpack<JoinChannelPacket>(
                packet.payload);

        m_server->joinChannel(
            m_user,
            req.channelId,
            req.password);

        m_server->printChannelWithUsersIn();
    }
    break;

    case PacketType::ChatMessage:
    {
        auto msg = PacketHelpers::unpack<SendMessagePacket>(packet.payload);

        qDebug() << "message received:"
                 << " text:" << msg.text
                 << " type:" << msg.type
                 << " mediapath:" << msg.mediaPath;

        m_server->broadcastMessage(m_user,msg);
    }

    break;

    default:
        break;
    }
}

void ClientSession::onDisconnected()
{
    if(m_user)
    {
        m_server->removeUser(
            m_user);
    }

    deleteLater();
}
