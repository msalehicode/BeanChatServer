#pragma once

#include <QObject>

#include <QTcpServer>
#include <QTcpSocket>

#include <QList>
#include <QHash>

#include <QDateTime>
#include <QDebug>

#include <QSqlQuery>
#include <QVariant>


#include <QCryptographicHash>
#include <QFile>
#include <QDir>

#include "../models/user.h"
#include "../models/channel.h"
#include "../models/database.h"

#include "../network/clientsession.h"

#include "../models/user.h"
#include "../models/channel.h"
#include "../network/udpserver.h"

//from BeanChatCommon
#include <protocol/Packet.h>
#include <protocol/Packets.h>
#include <protocol/PacketHelpers.h>
using namespace BeanChatCommon;


#include <QVersionNumber>

//make received avatar rounded
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QBuffer>


class ClientSession;

class Server : public QObject
{
    Q_OBJECT

public:
    const QString avatarDirectoryName = AVATARS_DIRECTORY_NAME;
    const QVersionNumber minimumVersion = QVersionNumber::fromString(CLIENT_MINIMUM_VERSION);

    explicit Server(Database* db, QObject* parent = nullptr);

    bool start(quint16 port,
               quint16 udpPort);

    UserModel* loginUser(
        const LoginRequestPacket &req,
        QTcpSocket* socket);

    UserModel* findUser(
        QTcpSocket* socket);

    void removeUser(UserModel* user);

    void removeSession(QTcpSocket *socket);
    void disconnectUser(UserModel *user, bool connectionLost=false);

    Channel* createChannel(
        const QString& name,
        const QString& password,
        bool saveChats,
        UserModel *owner=nullptr);

    int changeUserStatus(
        PacketType type,
        UserModel* user); //if status changed return 0/1 otherwise return -1 to say status not changed

    void saveMessage(
        quint64 channelId,
        quint64 senderId,
        const QString& text);


    QByteArray joinChannel(
        UserModel* user,
        quint64 channelId,
        const QString& password);

    QList<UserModel*> users() const;

    QByteArray buildServerState();


    //prints
    void printChannels();
    void printChannelWithUsersIn();
    void printUsers();


    Channel *updateChannel(quint64 channelId, const QString &name, const QString &pass, bool saveChats);
    bool updateChannel(Channel* channel, const QString &name, const QString &pass, bool saveChats);
    bool deleteChannel(Channel *channel);

    Channel *findChannelById(quint64 id);
    UserModel *findUser(quint64 userId);
    bool isPublicKeyInUse(const QByteArray &pubkey);



    //avatar
    QString generateAvatarHash(const QByteArray& avatarData); // usage:QString hash = generateAvatarHash(imageBytes);
    QByteArray imageFileToBytes(const QString &path);
    bool saveAvatarImage(const QString &serverDir, const QString &hash, const QByteArray &avatarData);
    bool deleteAvatar(const QString &serverDir, const QString &hash);
    QString updateUserAvatar(UserModel *user, const QByteArray &data, bool &removeOldAvatar); //if succeed reutrns a hash else empty string
    bool isAvatarHashUsedByAnotherUser(const QString& avatarHash);

    bool makeAvatarRounded(QByteArray &avatarData, int avatarSize=128);
    QString platformName();
    ServerInfo* info();

    bool updateUsername(UserModel *user, const QString &newUsername);
private slots:
    void onNewConnection();


private:
    ServerInfo m_info;

    quint64 m_nextUserId = 1;

    Database* m_db=nullptr;
    QTcpServer m_server;

    QHash<QTcpSocket*, ClientSession*> m_sessions;

    QList<UserModel*> m_users;

    QList<Channel*> m_channels;

    UdpServer* m_udpServer;
};
