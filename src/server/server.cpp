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

    //create some basic channels
    auto admins = new Channel;
    admins->id = 1;
    admins->name = "Admins";
    admins->saveChats=true;
    admins->password="admins";
    m_channels.push_back(admins);


    auto lobby = new Channel;
    lobby->id = 2;
    lobby->name = "Lobby";
    m_channels.push_back(lobby);



    auto dota2 = new Channel;
    dota2->id = 3;
    dota2->name = "Dota2";
    dota2->saveChats=true;
    m_channels.push_back(dota2);

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
        qInfo() << "failed to bind tcp port.";
        return false;
    }

    qDebug()
        << "TCP Listening on "
        << port;



    m_udpServer =
        new UdpServer(
            this,
            this);

    if(!m_udpServer->start(udpPort))
    {
        qInfo() << "failed to bind udp port.";
        return false;
    }

    return true;
}


void Server::onNewConnection()
{
    while (m_server.hasPendingConnections())
    {
        QTcpSocket *socket = m_server.nextPendingConnection();

        ClientSession *session = new ClientSession(socket, this);

        m_sessions.insert(socket, session);

        qDebug() << "Incoming connection";
    }
}

UserModel* Server::loginUser(
    const LoginRequestPacket& req,
    QTcpSocket* socket)
{
    //set essential data from this user, if needed to ban
    auto user = new UserModel;

    user->identity = req.identity;

    user->socket = socket;

    user->ip =socket->peerAddress().toString();
    user->port =socket->peerPort();
    user->appVersion = req.appVersion;
    user->buildType = req.buildType;
    user->machineId = req.machineId;
    user->machineName = req.machineName;
    user->osName = req.osName;
    user->osVersion = req.osVersion;

    user->id =m_nextUserId++;
    user->username = req.username;

    user->connectedSince = QDateTime::currentSecsSinceEpoch();

    user->currentChannel=nullptr;
    //dont set a channel to user.
    // user->currentChannel =
    //     m_channels.first();
    // user->currentChannel
    //     ->users
    //     .push_back(user);

    m_users.push_back(user);

    qDebug() << req.username << "logged in";

    return user;
}

UserModel *Server::findUser(QTcpSocket *socket)
{
    for(UserModel* usr : m_users)
    {
        if(usr->socket->peerAddress().toString() == socket->peerAddress().toString()
            && usr->socket->peerPort() == socket->peerPort())
        {
            return usr;
        }
    }
    return nullptr;
}

void Server::removeUser(UserModel *user)
{
    if (!user)
        return;

    if (user->currentChannel)
        user->currentChannel->users.removeAll(user);

    m_users.removeAll(user);

    delete user;
}

void Server::removeSession(QTcpSocket *socket)
{
    m_sessions.remove(socket);
}

void Server::disconnectUser(UserModel *user, bool connectionLost)
{
    ClientSession *session = m_sessions.value(user->socket);

    if (session)
        session->forceDisconnect(connectionLost);
}

Channel* Server::createChannel(
    const QString& name,
    const QString& password,
    bool saveChats,
    UserModel* owner)
{
    auto channel = new Channel;

    channel->id = m_channels.size() + 1;
    channel->name = name;
    channel->password = password;
    channel->saveChats = saveChats;

    if(owner)
        channel->owner=owner;
    else
        channel->owner=nullptr;

    m_channels.push_back(channel);

    qDebug()
        << "Channel created:"
        << channel->id
        << channel->name;

    return channel;
}

int Server::changeUserStatus(PacketType type,
                              UserModel* user)
{
    switch(type)
    {
        case PacketType::UserCameraClosed:
            //is actually status changed?
            if(!user->camera)
                return false;//nothing changed.

            user->camera = false;
            return 0;
        case PacketType::UserCameraOpened:

            //is actually status changed?
            if(user->camera)
                return false;//nothing changed.

            user->camera = true;
            return 1;
            break;

        case PacketType::UserMuted:
            //is actually status changed?
            if(user->muted)
                return false;//nothing changed.

            user->muted=true;
            return 1;
        case PacketType::UserUnmuted:
            //is actually status changed?
            if(!user->muted)
                return false;//nothing changed.

            user->muted=false;
            return 0;

        case PacketType::UserDeafened:
            //is actually status changed?
            if(user->deafened)
                return false;//nothing changed.

            user->deafened=true;
            return 1;
        case PacketType::UserUndeafened:
            //is actually status changed?
            if(!user->deafened)
                return false;//nothing changed.

            user->deafened=false;
            return 0;
        default:
            qDebug() << "undefined type for changeUserStatus..";
    }

    return -1;
}

QByteArray Server::joinChannel(
    UserModel* user,
    quint64 channelId,
    const QString& password)
{
    //find channel by id
    Channel* target = nullptr;
    for(auto channel : m_channels)
    {
        if(channel->id == channelId)
        {
            target = channel;
            break;
        }
    }
    if(!target)
    {
        qDebug() << "channel not found";
        return QByteArray();
    }

    if(!target->password.isEmpty()
         && target->password != password)
    {
        qDebug() << "wrong password";
        return QByteArray();
    }

    //check if user had a current channel, then remove him from that channel
    quint64 userOldChannel=-1;
    if(user->currentChannel)
    {
        userOldChannel = user->currentChannel->id;

        if(user->currentChannel)
            user->currentChannel->users.removeAll(user);
    }

    //add user to new channel
    target->users.push_back(user);
    user->currentChannel = target;

    qDebug()
        << user->username
        << "has left channel-id "
        << userOldChannel
        << " and joined"
        << target->id << "(" << target->name << ")";

    UserJoinedChannelPacket ujs;
    ujs.channelId = target->id;
    ujs.userId = user->id;
    ujs.oldChannelId = userOldChannel;


    return PacketHelpers::pack(ujs);
}

QList<UserModel *> Server::users() const
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
        info.isLocked = (channel->password.isNull() ? false : true);
        info.saveChats = channel->saveChats;

        state.channels.push_back(info);
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

        info.camera =
            user->camera;


        //fill some basic info of user's system to other users.
        info.appVersion =user->appVersion;
        info.buildType = user->buildType;
        info.osName= user->osName;
        info.osVersion = user->osVersion;

        qDebug() << "creating state, user " << info.username << " camera is " << info.camera;



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
        for(UserModel* c : channel->users)
            qDebug() << "     " << c->username;
    }
}

void Server::printUsers()
{
    qDebug() << "users:";
    for(UserModel* user: m_users)
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

