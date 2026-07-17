#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

#include "channel.h"
#include "user.h"
#include "attachment.h"
#include "message.h"

enum class UserField
{
    Username,
    AvatarHash,
    OldAvatarHash,

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
    QList<UserModel*> loadAllUsers();
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
    bool isAvatarHashUsedByAnotherUser(const QString& avatarHash);



    //attachment
    quint64 createAttachment(const Attachment& attachment);
    bool deleteAttachment(quint64 attachmentId);
    Attachment attachment(quint64 attachmentId);
    QList<Attachment> attachmentsForChannel(quint64 channelId);
    Attachment attachmentById(quint64 attachmentId);

    //message
    quint64 createMessage(const Message& message);

    QList<Message> loadMessages(
        quint64 channelId,
        int offset,
        int limit);

    bool editMessage(
        quint64 messageId,
        const QString& text);

    bool deleteMessage(
        quint64 messageId);

private:
    QSqlDatabase m_db;
};
