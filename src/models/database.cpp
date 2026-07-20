#include "database.h"

Database::Database()
{
}

bool Database::open()
{
    m_db =
        QSqlDatabase::addDatabase(
            "QSQLITE");

    m_db.setDatabaseName(
        DATABASE_FILE_NAME);

    if(!m_db.open())
    {
        qDebug()
        << m_db.lastError();

        return false;
    }

    return createTables();
}

bool Database::createTables()
{
    QSqlQuery query;

    query.exec(
       R"(
            CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,

                identity TEXT UNIQUE NOT NULL,
                username TEXT NOT NULL,
                avatarHash TEXT,
                oldAvatarHash TEXT,

                banned INTEGER NOT NULL DEFAULT 0,
                banExpiresAt INTEGER NOT NULL DEFAULT 0,
                banReason TEXT,

                isAdmin INTEGER NOT NULL DEFAULT 0,
                canTalk INTEGER NOT NULL DEFAULT 1,
                canChat INTEGER NOT NULL DEFAULT 1,
                canShareVideo INTEGER NOT NULL DEFAULT 1,
                canCreateChannel INTEGER NOT NULL DEFAULT 1,

                totalConnected INTEGER NOT NULL DEFAULT 0,

                createdAt DATETIME DEFAULT CURRENT_TIMESTAMP,
                lastLogin DATETIME DEFAULT CURRENT_TIMESTAMP,
                updatedAt DATETIME DEFAULT CURRENT_TIMESTAMP
            );
        )");

    query.exec(
        "CREATE TABLE IF NOT EXISTS channels ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "password TEXT,"
        "type INTEGER NOT NULL DEFAULT 1,"//0-> unknown, 1 -> voice, 2->text
        "ownerIdentity TEXT,"
        "saveChats INTEGER NOT NULL DEFAULT 0,"
        "displayOrder INTEGER NOT NULL DEFAULT 0,"
        "createdAt DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "updatedAt DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP"
        ");");

    query.exec(R"(
    CREATE TABLE IF NOT EXISTS messages
    (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        channelId INTEGER NOT NULL,
        senderId INTEGER NOT NULL,
        type INTEGER NOT NULL,
        text TEXT,
        attachmentId INTEGER DEFAULT NULL,
        edited INTEGER NOT NULL DEFAULT 0,
        deleted INTEGER NOT NULL DEFAULT 0,
        createdAt INTEGER,
        updatedAt INTEGER
    );
    )");

    query.exec(R"(
    CREATE TABLE IF NOT EXISTS attachments
    (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        channelId INTEGER NOT NULL,
        uploaderId INTEGER NOT NULL,
        originalFilename TEXT NOT NULL,
        storedFilename TEXT NOT NULL,
        mimeType TEXT,
        size INTEGER,
        sha256 BLOB,
        uploadedAt INTEGER
    );
    )");
    qDebug()
        << "Database initialized";

    return true;
}



bool Database::createChannel(Channel *channel)
{
    QSqlQuery query;

    query.prepare(
        "INSERT INTO channels "
        "(name,password,type,ownerIdentity,saveChats,displayOrder)"
        "VALUES"
        "(:name,:password,:type,:owner,:saveChats,:displayOrder)");

    query.bindValue(":name", channel->name);
    query.bindValue(":password", channel->password);
    query.bindValue(":type", static_cast<int>(channel->type));
    query.bindValue(":owner", channel->ownerIdentity);
    query.bindValue(":saveChats", channel->saveChats);
    query.bindValue(":displayOrder", channel->displayOrder);

    if(!query.exec())
    {
        qDebug() << query.lastError();
        return false;
    }

    channel->id = query.lastInsertId().toULongLong();

    return true;
}

bool Database::updateChannel(Channel *channel)
{
    QSqlQuery query;

    query.prepare(
        "UPDATE channels SET "
        "name=:name,"
        "password=:password,"
        "ownerIdentity=:owner,"
        "saveChats=:saveChats,"
        "displayOrder=:displayOrder,"
        "updatedAt=CURRENT_TIMESTAMP "
        "WHERE id=:id");

    query.bindValue(":id", channel->id);
    query.bindValue(":name", channel->name);
    query.bindValue(":password", channel->password);
    query.bindValue(":owner", channel->ownerIdentity);
    query.bindValue(":saveChats", channel->saveChats);
    query.bindValue(":displayOrder", channel->displayOrder);

    if(!query.exec())
    {
        qDebug() << query.lastError();
        return false;
    }

    return true;
}

bool Database::deleteChannel(quint64 id)
{
    QSqlQuery query;

    query.prepare(
        "DELETE FROM channels "
        "WHERE id=:id");

    query.bindValue(":id", id);

    if(!query.exec())
    {
        qDebug() << query.lastError();
        return false;
    }

    return true;
}


QList<Channel*> Database::loadChannels()
{
    QList<Channel*> channels;

    QSqlQuery query;

    query.exec(
        "SELECT * FROM channels "
        "ORDER BY displayOrder ASC");

    while(query.next())
    {
        Channel* channel = new Channel;

        channel->id =
            query.value("id").toULongLong();

        channel->name =
            query.value("name").toString();

        channel->password =
            query.value("password").toString();

        channel->type =
            static_cast<BeanChatCommon::ChannelType::Type>(query.value("type").toInt());

        channel->ownerIdentity =
            query.value("ownerIdentity").toString();

        channel->saveChats =
            query.value("saveChats").toBool();

        channel->displayOrder =
            query.value("displayOrder").toInt();

        channel->createdAt =
            query.value("createdAt").toDateTime();

        channel->updatedAt =
            query.value("updatedAt").toDateTime();

        channels.push_back(channel);
    }

    return channels;
}



QList<UserModel*> Database::loadAllUsers()
{
    QList<UserModel*> users;

    QSqlQuery query;

    if (!query.exec("SELECT * FROM users"))
    {
        qWarning() << "failed to load users error=" << query.lastError();
        return users;
    }

    while (query.next())
    {
        UserModel* user = new UserModel;

        user->id =
            query.value("id").toULongLong();

        user->identity =
            query.value("identity").toString();

        user->username =
            query.value("username").toString();

        user->avatarHash =
            query.value("avatarHash").toString();

        user->oldAvatarHash =
            query.value("oldAvatarHash").toString();

        user->isAdmin =
            query.value("isAdmin").toBool();

        user->banned =
            query.value("banned").toBool();

        user->banExpiresAt =
            query.value("banExpiresAt").toLongLong();

        user->banReason =
            query.value("banReason").toString();

        user->canTalk =
            query.value("canTalk").toBool();

        user->canChat =
            query.value("canChat").toBool();

        user->canShareVideo =
            query.value("canShareVideo").toBool();

        user->canCreateChannel =
            query.value("canCreateChannel").toBool();

        user->totalConnected =
            query.value("totalConnected").toULongLong();

        user->connected = false;

        users.push_back(user);
    }

    return users;
}

bool Database::loginUser(UserModel *user)
{
    QSqlQuery query;

    // Check if user exists
    query.prepare(
        "SELECT * "
        "FROM users "
        "WHERE identity=:identity");

    query.bindValue(
        ":identity",
        user->identity);

    if(!query.exec())
    {
        qDebug() << query.lastError();
        return false;
    }

    // User doesn't exist, create it
    if(!query.next())
    {
        qDebug()
        << "Creating new user:"
        << user->identity;

        QSqlQuery insert;

        insert.prepare(
            "INSERT INTO users "
            "(identity,username,avatarHash)"
            "VALUES"
            "(:identity,:username,:avatarHash)");

        insert.bindValue(
            ":identity",
            user->identity);

        insert.bindValue(
            ":username",
            user->username);

        insert.bindValue(
            ":avatarHash",
            user->avatarHash);

        if(!insert.exec())
        {
            qDebug() << insert.lastError();
            return false;
        }

        // Read the newly created user
        query.prepare(
            "SELECT * "
            "FROM users "
            "WHERE identity=:identity");

        query.bindValue(
            ":identity",
            user->identity);

        if(!query.exec())
        {
            qDebug() << query.lastError();
            return false;
        }

        if(!query.next())
        {
            qDebug() << "Failed to load newly created user.";
            return false;
        }
    }

    // Load persistent values
    user->id =
        query.value("id").toULongLong();

    user->avatarHash =
        query.value("avatarHash").toString();

    user->oldAvatarHash =
        query.value("oldAvatarHash").toString();

    user->isAdmin =
        query.value("isAdmin").toBool();

    user->banned =
        query.value("banned").toBool();

    user->banExpiresAt =
        query.value("banExpiresAt").toLongLong();

    user->banReason =
        query.value("banReason").toString();

    user->canTalk =
        query.value("canTalk").toBool();

    user->canChat =
        query.value("canChat").toBool();

    user->canShareVideo =
        query.value("canShareVideo").toBool();

    user->canCreateChannel =
        query.value("canCreateChannel").toBool();

    // Synchronize username
    QString dbUsername =
        query.value("username").toString();

    if(dbUsername != user->username)
    {
        qDebug()
        << "Updating username:"
        << dbUsername
        << "->"
        << user->username;

        updateUserField(
            user->identity,
            UserField::Username,
            user->username);
    }

    // Update login statistics
    QSqlQuery update;

    update.prepare(
        "UPDATE users SET "
        "totalConnected=totalConnected+1,"
        "lastLogin=CURRENT_TIMESTAMP "
        //"updatedAt=CURRENT_TIMESTAMP " //updatedAt is for update usernaem or.. by requestPacket
        "WHERE identity=:identity");

    update.bindValue(
        ":identity",
        user->identity);

    if(!update.exec())
    {
        qDebug() << update.lastError();
        return false;
    }

    // Automatically remove expired temporary bans
    if(user->banned &&
        user->banExpiresAt != 0 &&
        QDateTime::currentSecsSinceEpoch() >= user->banExpiresAt)
    {
        qDebug()
        << "Temporary ban expired for"
        << user->username;

        user->banned = false;
        user->banExpiresAt = 0;
        user->banReason.clear();

        setUserBan(
            user->identity,
            false,
            0,
            "");
    }

    return true;
}



bool Database::updateUserField(
    const QString& identity,
    UserField field,
    const QVariant& value)
{
    QString fieldName;

    switch(field)
    {
    case UserField::Username:
        fieldName = "username";
        break;

    case UserField::AvatarHash:
        fieldName = "avatarHash";
        break;

    case UserField::OldAvatarHash:
        fieldName = "oldAvatarHash";
        break;

    case UserField::IsAdmin:
        fieldName = "isAdmin";
        break;

    case UserField::Banned:
        fieldName = "banned";
        break;

    case UserField::BanExpiresAt:
        fieldName = "banExpiresAt";
        break;

    case UserField::BanReason:
        fieldName = "banReason";
        break;

    case UserField::CanTalk:
        fieldName = "canTalk";
        break;

    case UserField::CanChat:
        fieldName = "canChat";
        break;

    case UserField::CanShareVideo:
        fieldName = "canShareVideo";
        break;

    case UserField::CanCreateChannel:
        fieldName = "canCreateChannel";
        break;
    }

    QSqlQuery query;

    query.prepare(
        QString(
            "UPDATE users "
            "SET %1=:value,"
            "updatedAt=CURRENT_TIMESTAMP "
            "WHERE identity=:identity")
            .arg(fieldName));

    query.bindValue(":value", value);
    query.bindValue(":identity", identity);

    if(!query.exec())
    {
        qDebug() << query.lastError();
        return false;
    }

    return true;
}



bool Database::setUserBan(
    const QString& identity,
    bool banned,
    qint64 banExpiresAt,
    const QString& banReason)
{
    QSqlQuery query;

    query.prepare(
        "UPDATE users SET "
        "banned=:banned,"
        "banExpiresAt=:banExpiresAt,"
        "banReason=:banReason,"
        "updatedAt=CURRENT_TIMESTAMP "
        "WHERE identity=:identity");

    query.bindValue(
        ":banned",
        banned);

    query.bindValue(
        ":banExpiresAt",
        banExpiresAt);

    query.bindValue(
        ":banReason",
        banReason);

    query.bindValue(
        ":identity",
        identity);

    if(!query.exec())
    {
        qDebug() << query.lastError();
        return false;
    }

    return true;
}

bool Database::isAvatarHashUsedByAnotherUser(const QString &avatarHash)
{
    QSqlQuery query;

    query.prepare(
        "SELECT COUNT(*) "
        "FROM users "
        "WHERE avatarHash = ?");

    query.addBindValue(avatarHash);

    if (!query.exec())
    {
        qWarning() << "failed to read database for isAvatarHashUsedByAnotherUser error=" << query.lastError();
        return false;
    }

    if (!query.next())
        return false;

    return query.value(0).toInt() > 1;
}



quint64 Database::createAttachment(const Attachment &attachment)
{
    QSqlQuery query;

    query.prepare(R"(
        INSERT INTO attachments
        (
            channelId,
            uploaderId,
            originalFilename,
            storedFilename,
            mimeType,
            size,
            sha256,
            uploadedAt
        )
        VALUES
        (
            :channelId,
            :uploaderId,
            :originalFilename,
            :storedFilename,
            :mimeType,
            :size,
            :sha256,
            :uploadedAt
        )
    )");

    query.bindValue(":channelId", attachment.channelId);
    query.bindValue(":uploaderId", attachment.uploaderId);
    query.bindValue(":originalFilename", attachment.originalFilename);
    query.bindValue(":storedFilename", attachment.storedFilename);
    query.bindValue(":mimeType", attachment.mimeType);
    query.bindValue(":size", attachment.size);
    query.bindValue(":sha256", attachment.sha256);
    query.bindValue(":uploadedAt", attachment.uploadedAt.toSecsSinceEpoch());

    if (!query.exec())
    {
        qWarning() << query.lastError();
        return 0;
    }

    return query.lastInsertId().toULongLong();
}



bool Database::deleteAttachment(quint64 attachmentId)
{
    QSqlQuery query;

    query.prepare(
        "DELETE FROM attachments "
        "WHERE id=:id");

    query.bindValue(":id", attachmentId);

    if (!query.exec())
    {
        qWarning() << query.lastError();
        return false;
    }

    return true;
}


Attachment Database::attachment(quint64 attachmentId)
{
    Attachment attachment;

    QSqlQuery query;

    query.prepare(
        "SELECT * "
        "FROM attachments "
        "WHERE id=:id");

    query.bindValue(":id", attachmentId);

    if (!query.exec())
    {
        qWarning() << query.lastError();
        return attachment;
    }

    if (!query.next())
        return attachment;

    attachment.id = query.value("id").toULongLong();
    attachment.channelId = query.value("channelId").toULongLong();
    attachment.uploaderId = query.value("uploaderId").toULongLong();

    attachment.originalFilename =
        query.value("originalFilename").toString();

    attachment.storedFilename =
        query.value("storedFilename").toString();

    attachment.mimeType =
        query.value("mimeType").toString();

    attachment.size =
        query.value("size").toULongLong();

    attachment.sha256 =
        query.value("sha256").toByteArray();

    attachment.uploadedAt =
        QDateTime::fromSecsSinceEpoch(
            query.value("uploadedAt").toLongLong());

    return attachment;
}


QList<Attachment> Database::attachmentsForChannel(quint64 channelId)
{
    QList<Attachment> attachments;

    QSqlQuery query;

    query.prepare(
        "SELECT * "
        "FROM attachments "
        "WHERE channelId=:channelId "
        "ORDER BY uploadedAt ASC");

    query.bindValue(":channelId", channelId);

    if (!query.exec())
    {
        qWarning() << query.lastError();
        return attachments;
    }

    while (query.next())
    {
        Attachment attachment;

        attachment.id =
            query.value("id").toULongLong();

        attachment.channelId =
            query.value("channelId").toULongLong();

        attachment.uploaderId =
            query.value("uploaderId").toULongLong();

        attachment.originalFilename =
            query.value("originalFilename").toString();

        attachment.storedFilename =
            query.value("storedFilename").toString();

        attachment.mimeType =
            query.value("mimeType").toString();

        attachment.size =
            query.value("size").toULongLong();

        attachment.sha256 =
            query.value("sha256").toByteArray();

        attachment.uploadedAt =
            QDateTime::fromSecsSinceEpoch(
                query.value("uploadedAt").toLongLong());

        attachments.push_back(attachment);
    }

    return attachments;
}


Attachment Database::attachmentById(quint64 attachmentId)
{
    Attachment attachment;

    QSqlQuery query(m_db);

    query.prepare(R"(
        SELECT
            id,
            channelId,
            uploaderId,
            originalFilename,
            storedFilename,
            mimeType,
            size,
            sha256,
            uploadedAt
        FROM attachments
        WHERE id = ?
    )");

    query.addBindValue(attachmentId);

    if (!query.exec())
        return attachment;

    if (!query.next())
        return attachment;

    attachment.id = query.value(0).toULongLong();
    attachment.channelId = query.value(1).toULongLong();
    attachment.uploaderId = query.value(2).toULongLong();

    attachment.originalFilename = query.value(3).toString();
    attachment.storedFilename = query.value(4).toString();

    attachment.mimeType = query.value(5).toString();
    attachment.size = query.value(6).toULongLong();

    attachment.sha256 = query.value(7).toByteArray();
    attachment.uploadedAt = query.value(8).toDateTime();

    return attachment;
}


quint64 Database::createMessage(const Message &message)
{
    QSqlQuery query;

    query.prepare(R"(
        INSERT INTO messages
        (
            channelId,
            senderId,
            type,
            text,
            attachmentId,
            edited,
            deleted,
            createdAt,
            updatedAt
        )
        VALUES
        (
            :channelId,
            :senderId,
            :type,
            :text,
            :attachmentId,
            :edited,
            :deleted,
            :createdAt,
            :updatedAt
        )
    )");

    query.bindValue(":channelId", message.channelId);
    query.bindValue(":senderId", message.senderId);
    query.bindValue(":type", message.type);
    query.bindValue(":text", message.text);

    if (message.attachmentId == 0)
        query.bindValue(":attachmentId", QVariant(QVariant::ULongLong));
    else
        query.bindValue(":attachmentId", message.attachmentId);

    query.bindValue(":edited", message.edited);
    query.bindValue(":deleted", message.deleted);
    query.bindValue(":createdAt", message.createdAt.toSecsSinceEpoch());
    query.bindValue(":updatedAt", message.updatedAt.toSecsSinceEpoch());

    if (!query.exec())
    {
        qWarning() << query.lastError();
        return 0;
    }

    return query.lastInsertId().toULongLong();
}



QList<Message> Database::loadMessages(
    quint64 channelId,
    int offset,
    int limit)
{
    QList<Message> messages;

    QSqlQuery query;

    query.prepare(R"(
        SELECT *
        FROM messages
        WHERE channelId=:channelId
        ORDER BY id DESC
        LIMIT :limit OFFSET :offset
    )");

    query.bindValue(":channelId", channelId);
    query.bindValue(":limit", limit);
    query.bindValue(":offset", offset);

    if (!query.exec())
    {
        qWarning() << query.lastError();
        return messages;
    }

    while (query.next())
    {
        Message message;

        message.id = query.value("id").toULongLong();
        message.channelId = query.value("channelId").toULongLong();
        message.senderId = query.value("senderId").toULongLong();
        message.type = static_cast<BeanChatCommon::Msg::Type>(query.value("type").toInt());

        message.text = query.value("text").toString();

        if (!query.value("attachmentId").isNull())
            message.attachmentId =
                query.value("attachmentId").toULongLong();

        message.edited =
            query.value("edited").toBool();

        message.deleted =
            query.value("deleted").toBool();

        message.createdAt =
            QDateTime::fromSecsSinceEpoch(
                query.value("createdAt").toLongLong());

        message.updatedAt =
            QDateTime::fromSecsSinceEpoch(
                query.value("updatedAt").toLongLong());

        messages.push_back(message);
    }

    std::reverse(messages.begin(), messages.end());

    return messages;
}


bool Database::editMessage(
    quint64 messageId,
    const QString &text)
{
    QSqlQuery query;

    query.prepare(R"(
        UPDATE messages
        SET
            text=:text,
            edited=1,
            updatedAt=:updatedAt
        WHERE id=:id
    )");

    query.bindValue(":text", text);
    query.bindValue(":updatedAt",
                    QDateTime::currentSecsSinceEpoch());
    query.bindValue(":id", messageId);

    if (!query.exec())
    {
        qWarning() << query.lastError();
        return false;
    }

    return true;
}


bool Database::deleteMessage(quint64 messageId)
{
    QSqlQuery query;

    query.prepare(R"(
        UPDATE messages
        SET
            deleted=1,
            text='',
            updatedAt=:updatedAt
        WHERE id=:id
    )");

    query.bindValue(":updatedAt",
                    QDateTime::currentSecsSinceEpoch());
    query.bindValue(":id", messageId);

    if (!query.exec())
    {
        qWarning() << query.lastError();
        return false;
    }

    return true;
}
