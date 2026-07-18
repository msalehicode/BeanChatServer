#include "clientsession.h"
#include <protocol/ProtocolVersion.h>

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

ClientSession::~ClientSession()
{
    while (!m_uploads.isEmpty())
        destroyUpload(m_uploads.begin(), true);
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

    m_server->lastTcpActivity.restart(); //keep track of when was last time we sent something to everyone.

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
        if (size > BeanChatCommon::ProtocolLimits::MaxPacketSize)
        {
            qWarning() << "Packet too large from " << m_socket->peerAddress();
            m_server->disconnectUser(m_user,false);
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

void ClientSession::handleLoginProof(const QByteArray& payload)
{
    qDebug() << "login proof user...";
    auto proof = PacketHelpers::unpack<LoginPacket>(payload);

    qDebug() << "proof Signature:" << proof.payload.toBase64();

    bool ok = Crypto::verify(m_pendingLogin.publicKey,
                            m_pendingChallenge,
                            proof.payload);

    //check is login verified?
    if(!ok)
    {
        LoginResponsePacket resp;
        resp.accepted = false;
        resp.message = "failed to login, verification fails.";
        qDebug() << "failed to login, verification fails.";
        sendToSender(PacketType::LoginResponse, PacketHelpers::pack(resp));
        return;
    }

    //server login (insert or fetch data)
    QString errorMessage;
    m_user = m_server->loginUser(m_pendingLogin,m_socket,errorMessage);

    //clear.
    m_pendingLogin = {};
    m_pendingChallenge.clear();

    if(!m_user)
    {
        qDebug() << "faild to loginUser. server returnes null user";
        LoginResponsePacket resp;
        resp.accepted = false;
        resp.message = errorMessage;
        sendToSender(PacketType::LoginResponse, PacketHelpers::pack(resp));
        return;
    }

    //check is user banned?
    if(m_user->banned)
    {
        qint64 remaining = m_user->banExpiresAt - QDateTime::currentSecsSinceEpoch();

        LoginResponsePacket resp;
        resp.accepted = false;
        resp.message = "You are banned. Come back in " + formatRemainingTime(remaining)+".";
        sendToSender(PacketType::LoginResponse, PacketHelpers::pack(resp));
        return;
    }

    //send login response to sender
    LoginResponsePacket resp;
    resp.id = m_user->id; //tell him is id, to know when we say user 10 moved if it's equal to his id knows it's himself.
    resp.accepted=true; //it's accepted so user send udp login/register and ask for serverLatestState
    resp.message="logged in succesfully.";
    sendToSender(PacketType::LoginResponse, PacketHelpers::pack(resp));


    m_server->printUsers();


    //notify all users in server someone connected.
    UserConnectedPacket uc;
    uc.id = m_user->id;
    uc.username = m_user->username;
    uc.identity = m_user->identity;
    uc.status=m_user->status;
    uc.avatarHash = m_user->avatarHash;
    uc.appVersion = m_user->appVersion;
    uc.buildType = m_user->buildType;
    uc.osName = m_user->osName;
    uc.osVersion = m_user->osVersion;
    qDebug() << "user connected and reported his system info: " <<  uc.appVersion << "-" << uc.buildType << "-" << uc.osName << "-" << uc.osVersion;
    sendToEveryone(PacketType::UserConnected, PacketHelpers::pack(uc));
}

void ClientSession::handleLogin(const QByteArray& payload)
{
    auto req = PacketHelpers::unpack<LoginRequestPacket>(payload);

    qDebug() << "login user...";


    //check user veriosn is compatible?
    QVersionNumber clientVersion = QVersionNumber::fromString(req.appVersion);
    if(clientVersion < m_server->minimumVersion
        || req.appProtocolVersion < BeanChatCommon::Protocol::Version )
    {
        qDebug() << "Client version too old." << req.appVersion<< " protocol=" << req.appProtocolVersion;

        LoginResponsePacket resp;
        resp.accepted = false;
        resp.message = "Please update BeanChat, the server accepts minimum version: " CLIENT_MINIMUM_VERSION " min-protocol: "+QString::number(BeanChatCommon::Protocol::Version);
        sendToSender(PacketType::LoginResponse, PacketHelpers::pack(resp));

        return;
    }

    //reject empty username or identity
    req.username = req.username.trimmed(); //remove many whitespaces
    if(req.username.isEmpty() || req.publicKey.isEmpty())
    {
        LoginResponsePacket resp;
        resp.accepted=false;
        resp.message="Please fill your username or identity.";
        sendToSender(PacketType::LoginResponse, PacketHelpers::pack(resp));
        return;
    }

    //check for username length
    if(req.username.size() > ProtocolLimits::MaxUsernameLength)
    {
        LoginResponsePacket resp;
        resp.accepted=false;
        resp.message="Username length is more than max, please make it small.";
        sendToSender(PacketType::LoginResponse, PacketHelpers::pack(resp));
        return;
    }


    //check public key is valid?
    if(!Crypto::isValidPublicKey(req.publicKey))
    {
        LoginResponsePacket resp;
        resp.accepted = false;
        resp.message = "Invalid identity.";
        sendToSender(PacketType::LoginResponse,
                     PacketHelpers::pack(resp));
        return;
    }

    //reject duplicated identity
    if(m_server->isPublicKeyInUse(req.publicKey))
    {
        LoginResponsePacket resp;
        resp.accepted=false;
        resp.message="Identity is being used by someone else.";
        sendToSender(PacketType::LoginResponse, PacketHelpers::pack(resp));
        return;
    }


    // Save pending login information
    m_pendingLogin = req;
    qDebug() << "requset Public key :" << m_pendingLogin.publicKey.toBase64();


    //generate a challenge
    m_pendingChallenge = Crypto::randomBytes(32);
    qDebug() << "Challenge created:" << m_pendingChallenge.toBase64();


    //send challange to client
    LoginPacket challenge;
    challenge.payload = m_pendingChallenge;
    sendToSender(PacketType::LoginChallenge, PacketHelpers::pack(challenge));
}

quint64 ClientSession::nextUploadId()
{
    return m_nextUploadId++;
}

void ClientSession::destroyUpload(
    QHash<quint64, UploadSession*>::iterator it,
    bool removeFile)
{
    UploadSession *session = it.value();

    session->file.close();

    if (removeFile)
        session->file.remove();

    delete session;
    m_uploads.erase(it);
}

void ClientSession::processPacket(
    const Packet& packet)
{
    qDebug() << "processPacket: type=" << static_cast<int>(packet.type);

    switch(packet.type)
    {
    case PacketType::YesEverythingIsOk:
    {
        // m_server->lastTcpActivity.restart(); //i guess no need to perform action for this.
        break;
    }
    case PacketType::LoginRequest:
    {
        //check for version, username , identity/public-key are valid then make a challange send to client wait for response
        handleLogin(packet.payload);
        break;
    }
    case PacketType::LoginProof:
    {
        //check login proof
        handleLoginProof(packet.payload);
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
            if(m_user->currentChannel)
                us.userChannelId=m_user->currentChannel->id;
            else
                us.userChannelId=-1;
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

            m_server->printChannels();
        }

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
            //is this user owner of the channel OR is admin?
            if(m_user->isAdmin
                || (channel->owner ? channel->owner == m_user : false))
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
            //is this user owner of the channel OR is admin?
            if(m_user->isAdmin
                || (channel->owner ? channel->owner == m_user : false))
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
                    qDebug() << "failed to delete channel.";
            }
            else
                qDebug() << "insufficient permission to modify this channel";
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


        //check if content is empty and has no attach ignore it
        if(msg.text.isEmpty() && msg.attachmentId==0)
            break;

        //convert sendMessagePacket to ChatMessagePacket
        ChatMessagePacket cm;
        cm.senderId = m_user->id;
        cm.senderName = m_user->username;
        cm.type = msg.type;
        cm.messageId=-1; //later increase this for each channel messages
        cm.text = msg.text;
        cm.attachmentId = msg.attachmentId;

        //attach check other dont use this id easily
        // Attachment attachment;
        // if (msg.attachmentId != 0)
        // {
        //     if (!m_server->db()->findAttachment(msg.attachmentId, attachment))
        //         break;

        //     if (attachment.uploaderId != m_user->id)
        //         break;

        //     if (attachment.channelId != channel->id)
        //         break;
        // }

        qDebug() << "message received:"
                 << " text:" << msg.text
                 << " type:" << msg.type
                 << " attachmentId:" << msg.attachmentId;

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

        //if userId is BeanChatCommon::ReservedIds::ServerAvatar, means user asked server's avatar
        if(p.notFoundIds.contains(BeanChatCommon::ReservedIds::ServerAvatar))
        {
            ServerInfo* serverInfo = m_server->info();
            if(serverInfo)
            {
                temp.userId=BeanChatCommon::ReservedIds::ServerAvatar;
                temp.avatarHash=serverInfo->avatarHash;
                temp.imageData = m_server->imageFileToBytes(m_server->avatarDirectoryName+"/"+serverInfo->avatarHash+".png");
                if(!m_server->isAvatarHashUsedByAnotherUser(serverInfo->oldAvatarHash))
                {
                    temp.oldHash= serverInfo->oldAvatarHash; //to tell users delete this old avatar.
                }
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
                temp.oldHash = user->oldAvatarHash;
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
            case UpdateUserInfoType::Username:
            {
                qDebug() << "SERVER: update username received";

                //check username length
                if(p.payloadValue.length() > BeanChatCommon::ProtocolLimits::MaxUsernameLength)
                {
                    qDebug() << "oh no we dont accept username with this long length.";
                    return;
                }


                if(m_server->updateUsername(m_user,p.payloadValue))
                {
                    //notify everyone a user username updated.
                    UserInfoChangedPacket ui;
                    ui.userId = m_user->id;
                    ui.payloadValue = p.payloadValue;
                    ui.updateType = UpdateUserInfoType::Username;
                    sendToEveryone(PacketType::UserInfoChanged, PacketHelpers::pack(ui));
                    qDebug() << "notify everyone, user's username updated.";
                }

                break;
            }
            case UpdateUserInfoType::Avatar:
            {
                qDebug() << "SERVER: update avatar received size=" << p.paylaodData.size();

                //check data size is valid for avatar?
                if(p.paylaodData.size() > BeanChatCommon::ProtocolLimits::MaxPacketSize)
                {
                    qDebug() << "oh no we dont accept image with this large size.";
                    return;
                }

                //try to make picture rounded, its better do.
                if (!m_server->makeAvatarRounded(p.paylaodData))
                    qWarning() << "Failed to round avatar.";
                else
                    qDebug() << "avatar image rounded.";

                QString oldHash = m_user->avatarHash;//to store old hash value WHEN it isn't  used by more than one user, to tell users delete this old one from cached files
                bool removeOldAvatar=false;
                QString hashResult = m_server->updateUserAvatar(m_user,p.paylaodData,removeOldAvatar);
                //check is generated hash is valid?
                if(!hashResult.isEmpty())
                {
                    //notify everyone a user avatar updated.
                    UserInfoChangedPacket ui;
                    ui.userId = m_user->id;
                    ui.payloadValue = hashResult;
                    ui.updateType = UpdateUserInfoType::Avatar;
                    ui.payloadData = p.paylaodData;

                    if(removeOldAvatar)
                        ui.payloadSecondValue = oldHash; //to tell users remove old file due to privacy and less cache size on local files

                    sendToEveryone(PacketType::UserInfoChanged, PacketHelpers::pack(ui));
                    qDebug() << "notify everyone, user's avatar updated.";
                }
                break;
            }
            case UpdateUserInfoType::ActivityStatus:
            {
                qDebug() << "SERVER: update activity status received";

                if(m_server->updateUserActivityStatus(m_user,p.payloadValue))
                {
                    //notify everyone a user username updated.
                    UserInfoChangedPacket ui;
                    ui.userId = m_user->id;
                    ui.payloadValue = p.payloadValue;
                    ui.updateType = UpdateUserInfoType::ActivityStatus;
                    sendToEveryone(PacketType::UserInfoChanged, PacketHelpers::pack(ui));
                    qDebug() << "notify everyone, user's activity status updated.";
                }
            }
            default:
                qDebug() << "a UpdateUserInfo received but not supported for now, type=" << static_cast<int>(p.updateType);
                break;
        }

    }break;

    case PacketType::UploadFileBegin:
    {
        auto req = PacketHelpers::unpack<UploadFileBeginPacket>(packet.payload);

        UploadFileBeginResponsePacket response;

        response.success = false;
        response.uploadId = 0;

        if (!m_user)
        {
            response.error = "Not logged in.";
            sendToSender(PacketType::UploadFileBeginResponse,
                         PacketHelpers::pack(response));
            break;
        }

        Channel *channel = m_user->currentChannel;
        if (!channel)
        {
            response.error = "No channel.";
            sendToSender(PacketType::UploadFileBeginResponse,
                         PacketHelpers::pack(response));
            break;
        }



        //check file name
        if (req.filename.trimmed().isEmpty()
            || req.filename.length() > 255
            || req.filename.contains('/')
            || req.filename.contains('\\'))
        {
            response.error = "Invalid filename.";

            sendToSender(
                PacketType::UploadFileBeginResponse,
                PacketHelpers::pack(response));

            break;
        }


        //check file size:
        if (req.fileSize == 0)
        {
            response.error = "File is empty.";

            sendToSender(
                PacketType::UploadFileBeginResponse,
                PacketHelpers::pack(response));

            break;
        }

        if (req.fileSize > ProtocolLimits::MaxAttachmentSize)
        {
            response.error = "File is too large, max size="+
                             QString::number(BeanChatCommon::ProtocolLimits::MaxAttachmentSize);

            sendToSender(
                PacketType::UploadFileBeginResponse,
                PacketHelpers::pack(response));

            break;
        }

        //mimi check
        if (req.mimeType.length() > 128)
        {
            response.error = "Large mimiType.";

            sendToSender(
                PacketType::UploadFileBeginResponse,
                PacketHelpers::pack(response));

            break;
        }




        UploadSession *session = new UploadSession;

        session->uploadId = nextUploadId();
        session->channelId = channel->id;

        session->originalFilename = req.filename;
        session->mimeType = req.mimeType;

        session->expectedSize = req.fileSize;
        session->sha256 = req.sha256;

        //create dir if not exsits
        QDir dir(m_server->uploadsDirectory());
        if (!dir.exists())
            dir.mkpath(".");


        // Temporary filename for now
        QString tempPath = QString(m_server->uploadsDirectory()+"/%1.upload")
                               .arg(session->uploadId);

        session->file.setFileName(tempPath);

        if (!session->file.open(QIODevice::WriteOnly))
        {
            response.error = "Cannot create upload file.";

            delete session;

            sendToSender(PacketType::UploadFileBeginResponse,
                         PacketHelpers::pack(response));
            break;
        }

        response.success = true;
        response.uploadId = session->uploadId;

        m_uploads.insert(session->uploadId, session);

        sendToSender(PacketType::UploadFileBeginResponse,
                     PacketHelpers::pack(response));

        break;
    }

    case PacketType::UploadFileChunk:
    {
        auto chunk = PacketHelpers::unpack<UploadFileChunkPacket>(packet.payload);

        auto it = m_uploads.find(chunk.uploadId);

        if (it == m_uploads.end())
            break;

        //reject upload 0bytes foreever
        if (chunk.payload.isEmpty())
        {
            destroyUpload(it, true);
            break;
        }

        UploadSession *session = it.value();

        // Prevent client from sending more data than promised
        if (session->receivedSize + chunk.payload.size() > session->expectedSize)
        {
            qWarning() << "Upload exceeds expected size.";

            destroyUpload(it, true);
            break;
        }

        qint64 written = session->file.write(chunk.payload);

        if (written != chunk.payload.size())
        {
            qWarning() << "Failed writing upload chunk.";

            destroyUpload(it, true);
            break;
        }

        session->receivedSize += written;

        break;
    }

    case PacketType::UploadFileFinish:
    {
        auto finish =
            PacketHelpers::unpack<UploadFileFinishPacket>(
                packet.payload);

        auto it = m_uploads.find(finish.uploadId);

        if (it == m_uploads.end())
            break;

        UploadSession *session = it.value();

        UploadFileFinishResponsePacket response;
        response.success = false;
        response.attachmentId = 0;

        // Verify received size
        if (session->receivedSize != session->expectedSize)
        {
            qWarning() << "Upload incomplete.";

            response.error = "Upload incomplete.";

            sendToSender(
                PacketType::UploadFileFinishResponse,
                PacketHelpers::pack(response));

            destroyUpload(it, true);
            break;
        }


        session->file.flush();
        session->file.close();


        // Verify SHA256
        QFile file(session->file.fileName());

        if (!file.open(QIODevice::ReadOnly))
        {
            response.error = "Failed reading uploaded file.";

            sendToSender(
                PacketType::UploadFileFinishResponse,
                PacketHelpers::pack(response));

            destroyUpload(it, true);
            break;
        }


        QByteArray actualSha256 =
            QCryptographicHash::hash(
                file.readAll(),
                QCryptographicHash::Sha256);

        file.close();

        if (actualSha256 != session->sha256)
        {
            qWarning() << "SHA256 mismatch.";

            response.error = "Uploaded file is corrupted.";

            sendToSender(
                PacketType::UploadFileFinishResponse,
                PacketHelpers::pack(response));

            destroyUpload(it, true);
            break;
        }




        // Generate permanent filename
        QString storedFilename =
            QUuid::createUuid().toString(
                QUuid::WithoutBraces);

        QString finalPath =
            "uploads/" + storedFilename;

        if (!QFile::rename(session->file.fileName(), finalPath))
        {
            qWarning() << "Failed moving uploaded file.";

            response.error = "Failed storing file.";

            sendToSender(
                PacketType::UploadFileFinishResponse,
                PacketHelpers::pack(response));

            destroyUpload(it, true);
            break;
        }

        Attachment attachment;

        attachment.channelId = session->channelId;
        attachment.uploaderId = m_user->id;

        attachment.originalFilename = session->originalFilename;
        attachment.storedFilename = storedFilename;

        attachment.mimeType = session->mimeType;
        attachment.size = session->receivedSize;

        attachment.sha256 = session->sha256;
        attachment.uploadedAt = QDateTime::currentDateTimeUtc();

        quint64 attachmentId = m_server->db()->createAttachment(attachment);

        if (attachmentId == 0)
        {
            QFile::remove(finalPath);

            response.error = "Failed saving attachment.";

            sendToSender(
                PacketType::UploadFileFinishResponse,
                PacketHelpers::pack(response));

            destroyUpload(it, false);
            break;
        }

        qDebug() << "Upload completed:"
                 << attachment.originalFilename
                 << attachmentId;

        response.success = true;
        response.attachmentId = attachmentId;

        sendToSender(
            PacketType::UploadFileFinishResponse,
            PacketHelpers::pack(response));
        destroyUpload(it, false);

        break;
    }

    case PacketType::DownloadAttachment:
    {
        if (!m_user)
            break;

        auto req =
            PacketHelpers::unpack<DownloadAttachmentPacket>(
                packet.payload);

        Attachment attachment =
            m_server->db()->attachmentById(req.attachmentId);

        DownloadAttachmentBeginPacket begin;

        begin.success = false;
        begin.attachmentId = req.attachmentId;

        Channel *channel =m_server->findChannelById(attachment.channelId);
        if (!channel)
        {
            begin.error = "Channel not found.";

            sendToSender(
                PacketType::DownloadAttachmentBegin,
                PacketHelpers::pack(begin));

            break;
        }

        if (m_user->currentChannel != channel && !m_user->isAdmin)
        {
            begin.error = "Access denied.";

            sendToSender(
                PacketType::DownloadAttachmentBegin,
                PacketHelpers::pack(begin));

            break;
        }

        if (attachment.id == 0)
        {
            begin.error = "Attachment not found.";

            sendToSender(
                PacketType::DownloadAttachmentBegin,
                PacketHelpers::pack(begin));

            break;
        }

        QFile file(m_server->uploadsDirectory()+"/" + attachment.storedFilename);

        if (!file.open(QIODevice::ReadOnly))
        {
            begin.error = "Cannot open attachment.";

            sendToSender(
                PacketType::DownloadAttachmentBegin,
                PacketHelpers::pack(begin));

            break;
        }

        begin.success = true;
        begin.filename = attachment.originalFilename;
        begin.mimeType = attachment.mimeType;
        begin.size = attachment.size;
        begin.sha256 = attachment.sha256;

        sendToSender(
            PacketType::DownloadAttachmentBegin,
            PacketHelpers::pack(begin));

        const int ChunkSize = 64 * 1024;

        while (!file.atEnd())
        {
            DownloadAttachmentChunkPacket chunk;

            chunk.attachmentId = attachment.id;
            chunk.payload = file.read(ChunkSize);

            sendToSender(
                PacketType::DownloadAttachmentChunk,
                PacketHelpers::pack(chunk));
        }

        file.close();

        DownloadAttachmentFinishPacket finish;

        finish.attachmentId = attachment.id;

        sendToSender(
            PacketType::DownloadAttachmentFinish,
            PacketHelpers::pack(finish));

        break;
    }

    default:
        break;
    }
}


void ClientSession::forceDisconnect(bool connectionLost)
{
    qDebug() << "force disconnect user due to (connection lost) or (he sent a packet which hits ProtocolLimits::MaxPacketSize) or whatever...";
    m_connectionLost=connectionLost;
    m_socket->disconnectFromHost(); //would trigger onDisconnected()
}

QString ClientSession::formatRemainingTime(qint64 seconds)
{
    if (seconds <= 0)
        return "Expired";

    int days = seconds / 86400;
    seconds %= 86400;

    int hours = seconds / 3600;
    seconds %= 3600;

    int minutes = seconds / 60;

    if (days > 0)
        return QString("%1 day%2 %3 hour%4")
            .arg(days)
            .arg(days == 1 ? "" : "s")
            .arg(hours)
            .arg(hours == 1 ? "" : "s");

    if (hours > 0)
        return QString("%1 hour%2 %3 minute%4")
            .arg(hours)
            .arg(hours == 1 ? "" : "s")
            .arg(minutes)
            .arg(minutes == 1 ? "" : "s");

    return QString("%1 minute%2")
        .arg(minutes)
        .arg(minutes == 1 ? "" : "s");
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

        m_user->status=BeanChatCommon::Presence::Status::Offline;
        m_server->removeUser(m_user);
        m_user = nullptr;
    }

    m_server->removeSession(m_socket);

    m_socket->deleteLater();
    deleteLater();
}
