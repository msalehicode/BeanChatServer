#include "server.h"

#include "../network/clientsession.h"

#include "../models/user.h"
#include "../models/channel.h"

#include "../network/packethelpers.h"

#include <QDateTime>
#include <QDebug>

#include <QSqlQuery>
#include <QVariant>

Server::Server(
    QObject* parent)
    :
    QObject(parent)
{
    auto lobby =
        new Channel;

    lobby->id = 1;
    lobby->name = "Lobby";

    m_channels.push_back(
        lobby);
}

bool Server::start(
    quint16 port)
{
    connect(
        &m_server,
        &QTcpServer::newConnection,
        this,
        &Server::onNewConnection);

    if(!m_server.listen(
            QHostAddress::Any,
            port))
    {
        return false;
    }

    qDebug()
        << "Listening on"
        << port;

    return true;
}

void Server::onNewConnection()
{
    while(
        m_server.hasPendingConnections())
    {
        auto socket =
            m_server.nextPendingConnection();

        new ClientSession(
            socket,
            this);

        qDebug()
            << "Incoming connection";
    }
}

User* Server::loginUser(
    const QString& username,
    const QString& identity,
    QTcpSocket* socket)
{
    auto user =
        new User;

    user->id =
        m_nextUserId++;

    user->username =
        username;

    user->identity =
        identity;

    user->socket =
        socket;

    user->ip =
        socket->peerAddress()
            .toString();

    user->connectedSince =
        QDateTime::currentSecsSinceEpoch();

    user->currentChannel =
        m_channels.first();

    user->currentChannel
        ->users
        .push_back(user);

    m_users.push_back(
        user);

    qDebug()
        << username
        << "logged in";

    QString msg = user->username + " has connected";
    notifyEveryone(msg);

    printUsers();

    return user;
}

void Server::removeUser(
    User* user)
{
    if(!user)
        return;

    if(user->currentChannel)
    {
        user->currentChannel
            ->users
            .removeAll(user);
    }

    m_users.removeAll(
        user);

    qDebug()
        << user->username
        << "disconnected";

    QString msg = user->username + " has disconnected";
    notifyEveryone(msg);

    delete user;
}

Channel* Server::createChannel(
    const QString& name,
    const QString& password,
    bool permanentChat,
    bool temporaryChat)
{
    auto channel =
        new Channel;

    channel->id =
        m_channels.size() + 1;

    channel->name =
        name;

    channel->password =
        password;

    channel->permanentChat =
        permanentChat;

    channel->temporaryChat =
        temporaryChat;

    m_channels.push_back(
        channel);

    qDebug()
        << "Channel created:"
        << channel->id
        << channel->name;

    QString msg = channel->id + " " + channel->name + "channel has created.";
    notifyEveryone(msg);

    return channel;
}

bool Server::joinChannel(
    User* user,
    quint64 channelId,
    const QString& password)
{
    Channel* target =
        nullptr;

    for(auto channel :
         m_channels)
    {
        if(channel->id
            == channelId)
        {
            target =
                channel;

            break;
        }
    }

    if(!target)
    {
        qDebug()
        << "channel not found";
        broadcastMessage(user,"channel not found.");

        return false;
    }

    if(target->password
        != password)
    {
        qDebug()
        << "wrong password";
        broadcastMessage(user,"wrong password.");
        return false;
    }

    if(user->currentChannel)
    {
        user->currentChannel
            ->users
            .removeAll(user);
    }

    target->users.push_back(
        user);

    user->currentChannel =
        target;

    qDebug()
        << user->username
        << "joined"
        << target->name;

    QString joint = user->username + " joined " + target->name;
    notifyEveryone(joint);

    return true;
}


void Server::saveMessage(
    quint64 channelId,
    quint64 senderId,
    const QString& text)
{
    QSqlQuery query;

    query.prepare(
        "INSERT INTO messages "
        "(channelId,senderId,message)"
        "VALUES(?,?,?)");

    query.addBindValue(
        channelId);

    query.addBindValue(
        senderId);

    query.addBindValue(
        text);

    query.exec();
}

void Server::broadcastMessage(
    User* sender,
    const QString& text)
{
    if(!sender)
        return;

    auto channel =
        sender->currentChannel;

    if(!channel)
        return;

    ChatMessagePacket packet;

    packet.senderId =
        sender->id;

    packet.channelId =
        channel->id;

    packet.text =
        text;

    QByteArray payload =
        PacketHelpers::pack(
            packet);

    Packet networkPacket;

    networkPacket.type =
        PacketType::ChatMessage;

    networkPacket.payload =
        payload;

    QByteArray bytes =
        networkPacket.serialize();

    for(auto user :
         channel->users)
    {
        user->socket->write(
            bytes);
    }

    if(channel->permanentChat)
    {
        saveMessage(
            channel->id,
            sender->id,
            text);
    }

    qDebug()
        << sender->username
        << ":"
        << text;
}

void Server::notifyEveryone(const QString &text)
{
    SendMessagePacket sm;
    sm.text = text;

    Packet packet;
    packet.type = PacketType::ChatMessage;

    packet.payload = PacketHelpers::pack(sm);

    QByteArray bytes = packet.serialize();

    //write to all users.
    for(auto user : m_users)
    {
        user->socket->write(bytes);
    }
}
