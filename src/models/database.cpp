#include "database.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

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
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "identity TEXT UNIQUE,"
        "username TEXT"
        ");");

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
