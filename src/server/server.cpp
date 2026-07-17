#include "server.h"

Server::Server(Database *db,
               QObject* parent)
    :
    m_db(db),
    QObject(parent)
{
    //read channels from database
    m_channels = m_db->loadChannels();
    printChannels();


    //read saved server info
    //code here

    //set server info
    m_info.version="v"+QString::fromUtf8(SERVER_VERSION) + " on " + platformName();
    m_info.name="BeanChat Server";
    m_info.website="https://example.com";
    m_info.avatarHash="782f57381bb2e4678376cdd49dfe7afc6e3f6041689803af8fdd5bf7bdc9542d";
    m_info.startTime=QDateTime::currentDateTimeUtc();
    m_info.showOfflineUsers=true;
    m_info.maxUsers=32;


    //load all users
    m_allUsers = m_db->loadAllUsers();
}

QString Server::platformName()
{
    #ifdef Q_OS_ANDROID
        return "Android";
    #elif defined(Q_OS_WIN)
        return "Windows";
    #elif defined(Q_OS_IOS)
        return "IOS";
    #elif defined(Q_OS_MACOS)
        return "MacOS";
    #elif defined(Q_OS_LINUX)
        return "Linux";
    #elif
    #else
        return "Unknown";
    #endif
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

QString Server::uploadsDirectory() const
{
    return m_uploadsDirectoryName;
}

Database *Server::db() const
{
    return m_db;
}

QList<UserModel *> Server::allUsers() const
{
    return m_allUsers;
}

ServerInfo* Server::info()
{
    return &m_info;
}

UserModel* Server::findUserByIdentity(const QString& identity)
{
    for (UserModel* user : m_allUsers)
    {
        if (user->identity == identity)
            return user;
    }

    return nullptr;
}

UserModel* Server::loginUser(
    const LoginRequestPacket& req,
    QTcpSocket* socket,
    QString& errorMessage)
{
    QString identityBase64 = QString::fromUtf8(req.publicKey.toBase64());

    UserModel* user = findUserByIdentity(identityBase64);


    //check if server is full (some slots are reserved for admins)
    if (m_users.count() >= m_info.maxUsers)
    {
        errorMessage = "server is full";
        return nullptr;
    }

    if (!user || !user->isAdmin) //user is new OR isn't admin
    {
        if (m_users.count() >= m_info.maxUsers - m_info.reservedSlots)
        {
            errorMessage = "server is full";
            return nullptr;
        }
    }



    bool isNewUser=false; //a flag to know delete if login fails
    bool loggedIn=false;

    if(!user)
    {
        user = new UserModel;
        user->identity = identityBase64;
        user->username = req.username;
        isNewUser=true;
    }

    //make user to pass to m_db->login for accept or reject
    user->socket = socket;
    user->ip =socket->peerAddress().toString();
    user->port =socket->peerPort();
    user->appVersion = req.appVersion;
    user->buildType = req.buildType;
    user->machineId = req.machineId;
    user->machineName = req.machineName;
    user->osName = req.osName;
    user->osVersion = req.osVersion;
    user->connectedSince = QDateTime::currentSecsSinceEpoch();
    user->currentChannel=nullptr;

    //check if received status is valid or not
    qDebug() << "login user, received status=" << static_cast<int>(req.status);
    if(BeanChatCommon::isValidPresenceStatus(req.status))
        user->status=req.status;
    else
        user->status=BeanChatCommon::Presence::Status::Online;


    //if user is now register to database, otherwise user exists in m_allUsers
    if(isNewUser)
    {
        //check if user exists set data to user-> else insert and set to user-> if had sql error return false
        if(m_db->loginUser(user)) //NOTE: user would modify by this function
            loggedIn=true;
        else
            qWarning() << "failed to register new user internal error.";
    }
    else if (user->connected)
    {
        errorMessage="login failed, User already connected.";
    }
    else //user login is fine
    {
        //check if username diff, update username
        if(user->username != req.username)
        {
            if(!updateUsername(user,req.username))
            {
                qWarning() << "Failed to update username.";
                user->username = req.username; //failed to update username on database so simple just for now update it but its temporary
            }
        }
        loggedIn=true;
    }



    if(!loggedIn)
    {
        if(isNewUser)
            delete user;

        errorMessage= "failed to login user";
        return nullptr;
    }

    qDebug() << req.username << "logged in";

    //check does this user own a channel?! if yes update that channel's owner pointer
    for(Channel* channel : m_channels)
    {
        if(channel->ownerIdentity == user->identity)
        {
            channel->owner = user;
        }
    }

    if(isNewUser)
        m_allUsers.push_back(user);

    user->connected=true;
    m_users.push_back(user);
    return user;
}

UserModel *Server::findUser(quint64 userId)
{
    for(UserModel* usr : m_users)
    {
        if(usr->id == userId)
            return usr;
    }
    return nullptr;
}

bool Server::isPublicKeyInUse(const QByteArray &pubkey)
{
    for(UserModel* usr : m_users)
    {
        if(usr->identity == QString::fromUtf8(pubkey.toBase64()))
            return true;
    }
    return false;
}

void Server::removeUser(UserModel *user)
{
    if (!user)
        return;

    if (user->currentChannel)
        user->currentChannel->users.removeAll(user);

    //reset user's temp info from m_allUsers
    user->socket = nullptr;
    user->udpRegistered=false;
    user->ip.clear();
    user->port=0;
    user->udpAddress.clear();
    user->udpPort=0;
    user->appVersion ="";
    user->buildType = "";
    user->machineId = "";
    user->machineName = "";
    user->osName = "";
    user->osVersion = "";
    user->connectedSince =0;
    user->connected=false;
    user->currentChannel=nullptr;


    //remove user from m_users but it lives on m_allUsers
    m_users.removeAll(user);
}

void Server::removeSession(QTcpSocket *socket)
{
    m_sessions.remove(socket);
}

void Server::disconnectUser(UserModel *user, bool connectionLost)
{
    if(!user)
        return;

    //reset channel->owner's pointer if user owns a channel
    for(Channel* channel : m_channels)
    {
        if(channel->owner == user)
        {
            channel->owner = nullptr;
        }
    }

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

    channel->name = name;
    channel->password = password;
    channel->saveChats = saveChats;
    channel->displayOrder = m_channels.size();

    if(owner)
    {
        channel->owner=owner;
        channel->ownerIdentity = owner->identity;
    }
    else
        channel->owner=nullptr;


    if(!m_db->createChannel(channel))
    {
        qWarning()<< "failed to createChannel in db.";
        delete channel;
        return nullptr;
    }

    qDebug()
        << "Channel created:"
        << channel->id
        << channel->name;

    m_channels.push_back(channel);

    return channel;
}


Channel*  Server::updateChannel(
    quint64 channelId,
    const QString& name,
    const QString& pass,
    bool saveChats
    )
{
    Channel* channel = findChannelById(channelId);
    if(updateChannel(channel,name,pass,saveChats))
        return channel;

    return nullptr;
}

bool Server::updateChannel(Channel* channel,
                   const QString &name,
                   const QString &pass,
                   bool saveChats)
{
    if(channel)
    {
        channel->name = name;
        if(pass!="***") //pass === "***" -> dont change password.
            channel->password = pass;
        channel->saveChats = saveChats;

        //later update received channel's displayOrder too.
        //code here

        if(!m_db->updateChannel(channel))
        {
            qWarning()<< "failed to update channle in db.";
            return false;
        }
        return true;
    }
    return false;
}

bool Server::deleteChannel(Channel *channel)
{
    if (!channel)
        return false;

    int index = m_channels.indexOf(channel);
    if (index == -1)
        return false;

    if(!m_db->deleteChannel(channel->id))
    {
        qWarning() <<"failed to delete channel in db.";
        return false;
    }


    //remove all users in this channel
    for(UserModel* user : channel->users)
    {
        qDebug() << "user removed from channel";
        user->currentChannel = nullptr;
    }

    m_channels.removeAt(index);
    delete channel;

    return true;
}


Channel* Server::findChannelById(quint64 id)
{
    for (Channel* channel : m_channels)
    {
        if (channel->id == id)
            return channel;
    }

    return nullptr;
}

QString Server::generateAvatarHash(const QByteArray &avatarData)
{
    return QCryptographicHash::hash(avatarData, QCryptographicHash::Sha256).toHex();
}

QByteArray Server::imageFileToBytes(const QString &path)
{
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly))
        return {};

    return file.readAll();
}

bool Server::makeAvatarRounded(QByteArray &avatarData, int avatarSize)
{
    QImage image;

    if (!image.loadFromData(avatarData))
        return false;

    // Scale while preserving aspect ratio
    image = image.scaled(
        avatarSize,
        avatarSize,
        Qt::KeepAspectRatioByExpanding,
        Qt::SmoothTransformation);

    // Center crop
    QRect cropRect(
        (image.width() - avatarSize) / 2,
        (image.height() - avatarSize) / 2,
        avatarSize,
        avatarSize);

    image = image.copy(cropRect);

    // Create transparent destination
    QImage rounded(
        avatarSize,
        avatarSize,
        QImage::Format_ARGB32_Premultiplied);

    rounded.fill(Qt::transparent);

    QPainter painter(&rounded);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QPainterPath path;
    path.addEllipse(rounded.rect());

    painter.setClipPath(path);
    painter.drawImage(0, 0, image);
    painter.end();

    // Encode back into the same QByteArray
    avatarData.clear();

    QBuffer buffer(&avatarData);
    buffer.open(QIODevice::WriteOnly);

    return rounded.save(&buffer, "PNG");
}

bool Server::saveAvatarImage(const QString &serverDir, const QString &hash, const QByteArray &avatarData)
{
    QDir().mkpath(serverDir);

    QFile file(serverDir + "/" + hash + ".png");

    if (!file.open(QIODevice::WriteOnly))
        return false;

    file.write(avatarData);

    return true;
}

bool Server::deleteAvatar(const QString &serverDir, const QString &hash)
{
    return QFile::remove(serverDir + "/" + hash + ".png");
}


bool Server::updateUsername(UserModel* user, const QString& newUsername)
{
    if(user)
    {
        //check for bad words fitler list, replace those parts OR just deny request
        //code here



        //update user's avatarHash in database
        if(!m_db->updateUserField(user->identity, UserField::Username, newUsername))
        {
            qWarning() <<"failed to update username in database!";
            return false;
        }

        user->username = newUsername;
        return true;
    }
    return false;
}

bool Server::updateUserActivityStatus(UserModel* user, const QString& newStatus)
{
    if(user)
    {
        bool ok=false;
        BeanChatCommon::Presence::Status status = static_cast<BeanChatCommon::Presence::Status>(newStatus.toInt(&ok));
        if(ok)
        {
            if(BeanChatCommon::isValidPresenceStatus(status))
            {
                user->status = status;
                return true;
            }
        }
    }
    return false;
}

QString Server::updateUserAvatar(UserModel* user, const QByteArray &data, bool& removeOldAvatar)
{
    QString hash;
    if(user)
    {

        hash = generateAvatarHash(data); //generate hash for that avatar data.

        if(saveAvatarImage(avatarDirectoryName,hash,data))
        {
            qDebug() <<"avatar saved into server,local files";


            //check if anyone has this hash avatar? (TO PREVENT REMOVE OTHER USERS AVATARS WITH SAME PICTURE)
            if(!isAvatarHashUsedByAnotherUser(user->avatarHash))
             {
                    //delete old avatar file (hash.png)
                    if(!deleteAvatar(avatarDirectoryName, user->avatarHash))
                        qWarning() << "warning: couldn't to delete old avatar in server files.";


                    //update databse old hash avatar for user (to know later when someone connected tell him if avatar wasn't use by another user remove it to keep privacy)
                    user->oldAvatarHash = user->avatarHash;
                    //update user's OldAvatarHash in database
                    if(!m_db->updateUserField(user->identity, UserField::OldAvatarHash, user->avatarHash))
                    {
                        qWarning() <<"failed to update user oldAvatarHash in database!";
                    }

                    //report to caller of this function  need to remove old avatar
                    removeOldAvatar=true;
             }
            else //more than one user is using that avatar hash so don't report oldHash (if report, all users would remove that avatar locally) OR when error occured for database it return false
            {
                 //reset old hash for this user
                 user->oldAvatarHash = "";
                 //update user's OldAvatarHash in database
                 if(!m_db->updateUserField(user->identity, UserField::OldAvatarHash, user->oldAvatarHash))
                 {
                     qWarning() <<"failed to reset user oldAvatarHash in database!";
                 }
             }


            //check is hash valid
            if(!hash.isEmpty())
            {
                //update user's avatar
                user->avatarHash = hash;

                //update user's avatarHash in database
                if(!m_db->updateUserField(user->identity, UserField::AvatarHash, hash))
                {
                    qWarning() <<"failed to update user new avatarHash in database! but we continue anyway";
                }


                return hash;
            }
            else
                qDebug() << "generated hash is invalid.";
        }
        else
            qDebug() << "faield to save avatar into local files.";
    }
    else
        qDebug() << "invalid user to update avatar.";

    return hash;
}

bool Server::isAvatarHashUsedByAnotherUser(const QString &avatarHash)
{
    return m_db->isAvatarHashUsedByAnotherUser(avatarHash);
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
    quint64 userOldChannel=0;
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

    //fill serverInfo
    state.serverInfo = m_info;

    //channels
    for(auto channel : m_channels)
    {
        ChannelInfo info;

        info.id = channel->id;
        info.name = channel->name;
        info.isLocked = (channel->password.isEmpty() ? false : true);
        info.saveChats = channel->saveChats;

        state.channels.push_back(info);
    }

    //users
    const int maxShowOffline = (m_info.maxUsers>32 ? 50 : m_info.maxUsers);
    int offlineCount = 0;

    for(auto user : m_allUsers)
    {
        // User is not connected.
        if (!user->connected)
        {
            // Server doesn't show offline users.
            if (!m_info.showOfflineUsers)
                continue;

            // Limit number of offline users sent.
            if (offlineCount >= maxShowOffline)
                continue;

            ++offlineCount;
        }

        UserInfo info;

        info.id =
            user->id;

        info.username =
            user->username;

        info.identity =
            user->identity;

        info.status = ( user->connected ? user->status : BeanChatCommon::Presence::Status::Offline);

        info.avatarHash = user->avatarHash;

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

        qDebug() << "creating state, user " << info.username << " avatarHash="<< info.avatarHash;


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

void Server::printAllUsers()
{
    qDebug() << "all users:";
    for(UserModel* user: m_allUsers)
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

