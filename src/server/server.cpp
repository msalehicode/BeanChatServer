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
    quint16 port, quint16 udpPort)
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
        << "Listening on "
        << port;



    m_udpServer =
        new UdpServer(
            this,
            this);

    m_udpServer->start(
        udpPort);

    qDebug()
        << "udpvoice Listening on "
        << udpPort;

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
    //set essential data from this user, if needed to ban
    auto user = new User;

    user->identity = identity;

    user->socket =socket;

    user->ip =socket->peerAddress().toString();


    LoginResponsePacket resp;
    resp.accepted = true;
    resp.message = resp.accepted ? "OK" : "Rejected";

    if(resp.accepted)
    {
        user->id =
            m_nextUserId++;

        user->username =
            username;

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

        UserConnectedPacket uc;
        uc.id = user->id;
        uc.username = user->username;

        LoginResponsePacket lrp;
        lrp.id = user->id;
        lrp.accepted=true;
        lrp.message="logged in fine";
        Packet packet;
        packet.type = PacketType::LoginResponse;
        packet.payload = PacketHelpers::pack(lrp);
        QByteArray bytes = packet.serialize();


        sendToAll(PacketType::UserConnected, PacketHelpers::pack(uc), user, bytes);

        printUsers();
    }
    else
    {
        user->socket->write(PacketHelpers::pack(resp));
        qDebug() << "an invalid login detected.";
    }


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

    UserDisconnectedPacket ud;
    ud.id = user->id;
    sendToAll(PacketType::UserDisconnected, PacketHelpers::pack(ud));


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

    ChannelCreatedPacket cc;
    cc.id = channel->id;
    cc.name = channel->name;
    sendToAll(PacketType::ChannelCreated, PacketHelpers::pack(cc));

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
        // broadcastMessage(user,"channel not found.");

        return false;
    }

    if(target->password
        != password && !target->password.isEmpty())
    {
        qDebug()
        << "wrong password";
        // broadcastMessage(user,"wrong password.");
        return false;
    }

    quint64 userOldChannel = user->currentChannel->id;

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
        << "has left "
        << userOldChannel
        << " and joined"
        << target->name;


    UserJoinedChannelPacket ujs;
    ujs.channelId = target->id;
    ujs.userId = user->id;
    ujs.oldChannelId = userOldChannel;

    Packet packet;
    packet.type = PacketType::UserJoinedChannel;
    packet.payload = PacketHelpers::pack(ujs);

    QByteArray bytes = packet.serialize();
    for(auto user : m_users)
        user->socket->write(bytes);


    return true;
}

QList<User *> Server::users() const
{
    return m_users;
}

QByteArray Server::buildServerState()
{
    ServerStatePacket state;

    for(auto channel : m_channels)
    {
        ChannelInfo info;

        info.id = channel->id;
        info.name = channel->name;

        info.permanentChat =
            channel->permanentChat;

        info.temporaryChat =
            channel->temporaryChat;

        state.channels.push_back(
            info);
    }

    for(auto user : m_users)
    {
        UserInfo info;

        info.id =
            user->id;

        info.username =
            user->username;

        info.channelId =
            user->currentChannel
                ? user->currentChannel->id
                : 0;

        info.muted =
            user->muted;

        info.deafened =
            user->deafened;

        state.users.push_back(
            info);
    }

    return PacketHelpers::pack(state);
}

void Server::printChannels()
{
    qDebug() << "channels:";
    for(Channel* channel: m_channels)
    {
        qDebug() << channel->id << " " << channel->name << " " << channel->password;
    }
}

void Server::printChannelWithUsersIn()
{
    qDebug() << "channels:";
    for(Channel* channel: m_channels)
    {
        qDebug() << channel->id << " " << channel->name << " " << channel->password;
        for(User* c : channel->users)
            qDebug() << "     " << c->username;
    }
}

void Server::printUsers()
{
    qDebug() << "users:";
    for(User* user: m_users)
    {
        qDebug() << user->id << " " << user->username << " " << user->identity << " " << user->ip;
    }
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

    // packet.channelId =
    //     channel->id;

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

void Server::broadcastMessage(User *sender, SendMessagePacket &message)
{
    if(!sender)
        return;

    auto channel =
        sender->currentChannel;

    if(!channel)
        return;


    //convert send message to chatmessage.
    ChatMessagePacket msg;


    msg.senderId = sender->id;
    msg.messageId = 0; //later change this -------------------------------------------
    msg.text = message.text;
    msg.type = static_cast<ChatMessagePacket::Type>(message.type);
    msg.mediaPath = message.mediaPath;
    // message.channelId = channel->id;
    msg.timestamp = QDateTime::currentDateTime();


    QByteArray payload = PacketHelpers::pack(msg);
    Packet networkPacket;
    networkPacket.type = PacketType::ChatMessage;
    networkPacket.payload = payload;
    QByteArray bytes = networkPacket.serialize();

    for(auto user : channel->users)
    {
        user->socket->write(bytes);
    }

    // if(channel->permanentChat)
    // {
    //     saveMessage(
    //         channel->id,
    //         sender->id,
    //         text);
    // }

    qDebug()
        << sender->username
        << ":"
        << message.text;
}



void Server::sendToAll(PacketType pt, const QByteArray& packedData,
                       User* exceptThis, QByteArray exceptData)
{
    Packet packet;
    packet.type = pt;
    packet.payload = packedData;

    QByteArray bytes = packet.serialize();
    if(exceptThis==nullptr)
    {
        for(auto user : m_users)
            user->socket->write(bytes);
    }
    else
    {
        for(auto user : m_users)
        {
            if(user->id == exceptThis->id)
                user->socket->write(exceptData);
            else
                user->socket->write(bytes);
        }
    }

}

void Server::sendToUser(User *receiver, const QByteArray &packedData)
{
    if(!receiver)
        return;

    for(User* user : m_users)
    {
        if(user->id==receiver->id)
        {
            user->socket->write(packedData);
            return;
        }
    }
    qDebug() << "user not found to send .. id=" << receiver->id;
    return;
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
