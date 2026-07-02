#include "clientsession.h"

#include "../network/packethelpers.h"
#include "../network/packets.h"

#include "../server/server.h"

#include "../models/user.h"

#include <QDebug>

constexpr quint32 tcpMaxPacketSize = 50 * 1024 * 1024; // 50 MB

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

UserModel *ClientSession::user() const
{
    return m_user;
}


void ClientSession::sendToSender(
    PacketType type,
    const QByteArray& payload)
{
    Packet packet;

    packet.type = type;
    packet.payload = payload;

    m_socket->write(
        packet.serialize());
}

void ClientSession::sendToEveryone(
    PacketType type,
    const QByteArray &payload)
{
    Packet packet;
    packet.type = type;
    packet.payload = payload;

    QByteArray bytes = packet.serialize();

    for (UserModel *user : m_server->users())
    {
        if (user->socket)
            user->socket->write(bytes);
    }
}

void ClientSession::sendToEveryoneExceptSender(
    PacketType type,
    const QByteArray &payload)
{
    Packet packet;
    packet.type = type;
    packet.payload = payload;

    QByteArray bytes = packet.serialize();

    for (UserModel *user : m_server->users())
    {
        if (!user->socket)
            continue;

        if (user == m_user)
            continue;

        user->socket->write(bytes);
    }
}


void ClientSession::sendToChannel(
    PacketType type,
    const QByteArray &payload)
{
    if (!m_user || !m_user->currentChannel)
        return;

    Packet packet;
    packet.type = type;
    packet.payload = payload;

    QByteArray bytes = packet.serialize();

    for (UserModel *user : m_user->currentChannel->users)
    {
        if (user->socket)
            user->socket->write(bytes);
    }
}


void ClientSession::onReadyRead()
{
    m_buffer.append(m_socket->readAll());

    while(true)
    {
        if(m_buffer.size() < 6)
            return;

        QDataStream header(m_buffer);

        quint16 type;
        quint32 size;

        header >> type;
        header >> size;

        //check for packet size dont exceed from the max size
        if (size > tcpMaxPacketSize)
        {
            qWarning() << "Packet too large from " << m_socket->peerAddress();
            forceDisconnect(false);
            return;
        }

        if(m_buffer.size() < (6 + size))
            return;



        QByteArray packetBytes = m_buffer.left(6 + size);

        m_buffer.remove(
            0,
            6 + size);

        Packet packet = Packet::deserialize(packetBytes);

        processPacket(packet);
    }
}

void ClientSession::handleLogin(const QByteArray& payload)
{
    auto req = PacketHelpers::unpack<LoginRequestPacket>(payload);

    qDebug() << "login user...";

    //check user veriosn compatibility?
    //code here

    //authentication
    if(req.username.isEmpty() || req.identity.isEmpty())
    {
        LoginResponsePacket resp;
        resp.accepted=false;
        resp.message="login failed due to username or identity is empty.";
        sendToSender(PacketType::LoginResponse, PacketHelpers::pack(resp));
        return;
    }


    m_user = m_server->loginUser(req,m_socket);
    if(!m_user)
    {
        qDebug() << "faild to loginUser. server returnes null user.";
        return;
    }

    //send login response to sender
    LoginResponsePacket lrp;
    lrp.id = m_user->id; //tell him is id, to know when we say user 10 moved if it's equal to his id knows it's himself.
    lrp.accepted=true; //it's accepted so user send udp login/register and ask for serverLatestState
    lrp.message="logged in succesfully.";
    sendToSender(PacketType::LoginResponse, PacketHelpers::pack(lrp));


    m_server->printUsers();


    //notify all users in server someone connected.
    UserConnectedPacket uc;
    uc.id = m_user->id;
    uc.username = m_user->username;
    uc.avatarHash = m_user->avatarHash;
    uc.appVersion = m_user->appVersion;
    uc.buildType = m_user->buildType;
    uc.osName = m_user->osName;
    uc.osVersion = m_user->osVersion;
    qDebug() << "user connected and reported his system info: " <<  uc.appVersion << "-" << uc.buildType << "-" << uc.osName << "-" << uc.osVersion;
    sendToEveryone(PacketType::UserConnected, PacketHelpers::pack(uc));
}

void ClientSession::processPacket(
    const Packet& packet)
{
    qDebug() << "processPacket: type=" << static_cast<int>(packet.type);

    switch(packet.type)
    {
    case PacketType::LoginRequest:
    {
        handleLogin(packet.payload);
        break;
    }
    case PacketType::RequestServerState:
    {
        //check for user logged in?
        if(!m_user)
            break;

        sendToSender(PacketType::ServerState, m_server->buildServerState());
        break;
    }
    case PacketType::UserCameraClosed:
    case PacketType::UserCameraOpened:
    case PacketType::UserMuted:
    case PacketType::UserUnmuted:
    case PacketType::UserDeafened:
    case PacketType::UserUndeafened:
    {
        //check for user logged in?
        if(!m_user)
            break;

        auto req = PacketHelpers::unpack<UserStatusChangedPacket>(packet.payload);

        //check if user status changed?
        bool status = m_server->changeUserStatus(packet.type, m_user);
        if(status == 0 || status == 1)
        {
            //send change to everyone
            UserStatusChangedPacket us;
            us.userId=m_user->id;
            us.userChannelId=m_user->currentChannel->id;
            us.status= status;
            sendToEveryone(packet.type, PacketHelpers::pack(us));
        }
        break;
    }

    case PacketType::CreateChannel:
    {
        //check for user logged in?
        if(!m_user)
            break;

        //check for has user permission to create channel
        //code here


        auto req = PacketHelpers::unpack<CreateChannelPacket>(packet.payload);

        Channel* channelCreated = m_server->createChannel(
            req.name,
            req.password,
            req.saveChats,
            m_user); //m_user as owner

        if(channelCreated)
        {
            ChannelCreatedPacket cc;
            cc.id = channelCreated->id;
            cc.name = channelCreated->name;
            cc.saveChats = channelCreated->saveChats;

            if(!channelCreated->password.isEmpty())
                cc.isLocked=true;

            sendToEveryone(PacketType::ChannelCreated, PacketHelpers::pack(cc));
        }

        m_server->printChannels();
        break;
    }

    case PacketType::UpdateChannel:
    {
        //check for user logged in?
        if(!m_user)
            break;

        auto req = PacketHelpers::unpack<UpdateChannelPacket>(packet.payload);


        Channel* channel = m_server->findChannelById(req.channelId);

        //check for has user permission to update this channel
        if(channel)
        {
            // if(channel->owner)
            {
                //is this user owner of the channel OR is admin?
                if(m_user->isAdmin) //|| channel->owner->id == m_user->id)
                {
                    //update channel.
                    if(m_server->updateChannel(channel, req.name, req.password, req.saveChats))
                    {
                        ChannelUpdatedPacket c;
                        c.channelId = channel->id;
                        c.name = channel->name;
                        c.isLocked = channel->password.isEmpty() ? false : true;
                        c.saveChats = channel->saveChats;

                        qDebug() << "channel " << channel->name << "(" << channel->id << ") has updated";
                        sendToEveryone(PacketType::ChannelUpdated, PacketHelpers::pack(c));
                    }
                    else
                        qDebug() << "failed to update channel.";
                }
                else
                    qDebug() << "insufficient permission to modify this channel";
            }
        }


        break;
    }


    case PacketType::DeleteChannel:
    {
        //check for user logged in?
        if(!m_user)
            break;

        auto req = PacketHelpers::unpack<DeleteChannelPacket>(packet.payload);


        Channel* channel = m_server->findChannelById(req.channelId);

        //check for has user permission to update this channel
        if(channel)
        {
            // if(channel->owner)
            {
                //is this user owner of the channel OR is admin?
                if(m_user->isAdmin) // || channel->owner->id == m_user->id)
                {
                    //delete channel.
                    if(m_server->deleteChannel(channel)) //watch out, we deleted channel pointer too
                    {
                        DeleteChannelPacket c;
                        c.channelId = req.channelId;

                        qDebug() << "channel " << req.channelId << " deleted";
                        sendToEveryone(PacketType::ChannelDeleted, PacketHelpers::pack(c));
                    }
                    else
                        qDebug() << "failed to update channel.";
                }
                else
                    qDebug() << "insufficient permission to modify this channel";
            }
        }

        break;
    }

    case PacketType::MoveUser:
    {
        //check for user logged in?
        if(!m_user)
            break;

        auto req = PacketHelpers::unpack<MoveUserPacket>(packet.payload);

        //prevent selfmove,
        if(req.userId == m_user->id)
        {
            QByteArray channelResponse = m_server->joinChannel(m_user, req.channelId, req.channelPassword);
            if(channelResponse.isEmpty())
            {
                qDebug() << "notify user, channel not found or password is incorrect";
                break;
            }

            sendToEveryone(PacketType::UserJoinedChannel, channelResponse);

            m_server->printChannelWithUsersIn();
            break;
        }



        //check permission
        if(m_user->isAdmin)
        {
            UserModel* targetUser = m_server->findUser(req.userId);
            if(targetUser)
            {
                QByteArray channelResponse = m_server->joinChannel(targetUser, req.channelId, req.channelPassword);
                if(channelResponse.isEmpty())
                {
                    qDebug() << "notify user, channel not found or password is incorrect";
                    break;
                }

                sendToEveryone(PacketType::UserMoved, channelResponse);

                m_server->printChannelWithUsersIn();
            }
            else
                qDebug() << "invalid user to move";

        }
        else
            qDebug() << "dont have permission to move user.";



        break;
    }


    case PacketType::JoinChannel:
    {
        //check for user logged in?
        if(!m_user)
            break;

        auto req = PacketHelpers::unpack<JoinChannelPacket>(packet.payload);

        QByteArray channelResponse = m_server->joinChannel(m_user, req.channelId, req.password);
        if(channelResponse.isEmpty())
        {
            qDebug() << "notify user, channel not found or password is incorrect";
            break;
        }

        sendToEveryone(PacketType::UserJoinedChannel, channelResponse);

        m_server->printChannelWithUsersIn();
        break;
    }


    case PacketType::ChatMessage:
    {
        //check for user logged in?
        if(!m_user)
            break;

        //check if user has a channel
        Channel* channel = m_user->currentChannel;
        if(!channel)
            break;

        //check has user permission to chat in channel?
        //code here


        //do unpack
        auto msg = PacketHelpers::unpack<SendMessagePacket>(packet.payload);


        //check if content is empty
        if(msg.text.isEmpty())
            break;

        //convert sendMessagePacket to ChatMessagePacket
        ChatMessagePacket cm;
        cm.senderId = m_user->id;
        cm.senderName = m_user->username;
        cm.type = static_cast<ChatMessagePacket::Type>(msg.type);
        cm.messageId=-1; //later increase this for each channel messages
        cm.text = msg.text;
        cm.mediaPath = msg.mediaPath;

        qDebug() << "message received:"
                 << " text:" << msg.text
                 << " type:" << msg.type
                 << " mediapath:" << msg.mediaPath;

        //save message if needed
        if(channel->saveChats)
        {
            qDebug() << "save message for channel";
            // m_server->saveMessage(
            //     channel->id,
            //     m_user->id,
            //     msg.text);
        }

        sendToChannel(PacketType::ChatMessage, PacketHelpers::pack(cm));
        break;
    }



    case PacketType::RequestAvatars:
    {
        //check for user logged in?
        if(!m_user)
            break;

        auto p = PacketHelpers::unpack<RequestAvatarsPacket>(packet.payload);

        qDebug() << "request avatars received not found ids count=" << p.notFoundIds.count();

        ResponseAvatarsPacket ra;


        //find and fill users' avatars
        UserAvatar temp;

        //if userId is RESERVED_TO_ASK_SERVERS_AVATAR, means user asked server's avatar
        if(p.notFoundIds.contains(RESERVED_TO_ASK_SERVERS_AVATAR))
        {
            ServerInfo* serverInfo = m_server->info();
            if(serverInfo)
            {
                temp.userId=RESERVED_TO_ASK_SERVERS_AVATAR;
                temp.avatarHash=serverInfo->avatarHash;
                temp.imageData = m_server->imageFileToBytes(m_server->avatarDirectoryName+"/"+serverInfo->avatarHash+".png");
                temp.oldHash= serverInfo->oldAvatarHash;//to tell users delete this old avatar
                qDebug() << "user asked servers avatar. filled fine :)";
                ra.avatars.append(temp);
            }
        }

        for(UserModel* user : m_server->users())
        {
            //find asked users by their Id
            if(p.notFoundIds.contains(user->id))
            {
                //reset data.
                temp.clear();

                //fill data
                temp.userId = user->id;
                temp.avatarHash = user->avatarHash;
                // temp.oldHash = ""; // ------------------- LATER need more work because server sent new hash so if user coulnt find that hash.png in cached so would ask for new avatar so we have a file leak that old hash.png wont delete anytime.
                temp.imageData = m_server->imageFileToBytes(m_server->avatarDirectoryName+"/"+user->avatarHash+".png");
                ra.avatars.append(temp);
            }
        }

        //send avatars.
        sendToSender(PacketType::ResponseAvatars, PacketHelpers::pack(ra));
        qDebug() << "response avatars sent to user.";
    }break;

    case PacketType::UpdateUserInfo:
    {
        //check for user logged in?
        if(!m_user)
            break;

        auto p = PacketHelpers::unpack<UpdateUserInfoPacket>(packet.payload);

        qDebug() << "request update my info received update type= "
                 << static_cast<int>(p.updateType) << "data size: " << p.paylaodData.size();

        switch(p.updateType)
        {
            case UpdateUserInfoType::Avatar:
            {
                qDebug() << "SERVER: update avatar received size=" << p.paylaodData.size();

                //check data size is valid for avatar?
                //code here...

                //try to make picture rounded, its better do.
                if (!m_server->makeAvatarRounded(p.paylaodData))
                    qWarning() << "Failed to round avatar.";
                else
                    qDebug() << "avatar image rounded.";

                QString oldHash = m_user->avatarHash;//to store old hash value to tell users delete this old one from cached files
                QString hashResult = m_server->updateUserAvatar(m_user,p.paylaodData);
                //check is generated hash is valid?
                if(!hashResult.isEmpty())
                {
                    //notify everyone a user avatar updated.
                    UserInfoChangedPacket ui;
                    ui.userId = m_user->id;
                    ui.payloadValue = hashResult;
                    ui.updateType = UpdateUserInfoType::Avatar;
                    ui.payloadData = p.paylaodData;
                    ui.payloadSecondValue = oldHash; //to tell users remove old file due to privacy and less cache size on local files
                    sendToEveryone(PacketType::UserInfoChanged, PacketHelpers::pack(ui));
                    qDebug() << "notify everyone, user's avatar updated.";
                }
                break;
            }
            default:
                qDebug() << "a UpdateUserInfo received but not supported for now, type=" << static_cast<int>(p.updateType);
                break;
        }

    }break;


    default:
        break;
    }
}


void ClientSession::forceDisconnect(bool connectionLost)
{
    qDebug() << "force disconnect user due to (connection lost) or (he sent a packet which hits tcpMaxPacketSize).";
    m_connectionLost=connectionLost;
    m_socket->disconnectFromHost();
}


void ClientSession::onDisconnected()
{

    if (m_user)
    {
        UserDisconnectedPacket dc;
        dc.id = m_user->id;
        dc.wasConnectionLost = m_connectionLost;

        qDebug() << "user " << m_user->username <<  "(" << m_user->id << ")  Disconnected.";
        sendToEveryone(PacketType::UserDisconnected, PacketHelpers::pack(dc));

        m_server->removeUser(m_user);
        m_user = nullptr;
    }

    m_server->removeSession(m_socket);

    m_socket->deleteLater();
    deleteLater();
}
