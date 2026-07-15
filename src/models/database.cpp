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
        "ownerIdentity TEXT,"
        "saveChats INTEGER NOT NULL DEFAULT 0,"
        "displayOrder INTEGER NOT NULL DEFAULT 0,"
        "createdAt DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "updatedAt DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP"
        ");");

    query.exec(
        "CREATE TABLE IF NOT EXISTS messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "channelId INTEGER,"
        "senderId INTEGER,"
        "message TEXT,"
        "createdAt DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");");

    qDebug()
        << "Database initialized";

    return true;
}



bool Database::createChannel(Channel *channel)
{
    QSqlQuery query;

    query.prepare(
        "INSERT INTO channels "
        "(name,password,ownerIdentity,saveChats,displayOrder)"
        "VALUES"
        "(:name,:password,:owner,:saveChats,:displayOrder)");

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
