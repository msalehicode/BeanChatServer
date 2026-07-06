#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

#include "channel.h"
#include "user.h"

enum class UserField
{
    Username,
    AvatarHash,

    IsAdmin,

    Banned,
    BanExpiresAt,
    BanReason,

    CanTalk,
    CanChat,
    CanShareVideo,
    CanCreateChannel
};

class Database
{
public:
    Database();

    bool open();

    bool createTables();

    //channel
    bool createChannel(Channel* channel);
    bool updateChannel(Channel* channel);
    bool deleteChannel(quint64 id);
    QList<Channel*> loadChannels();



    //user
    bool loginUser(UserModel* user);
    bool updateUserField(
        const QString& identity,
        UserField field,
        const QVariant& value);
    bool setUserBan(
        const QString& identity,
        bool banned,
        qint64 banExpiresAt,
        const QString& banReason);

private:
    QSqlDatabase m_db;
};
