#pragma once

#include <QObject>

#include <QTcpServer>
#include <QTcpSocket>

#include <QList>
#include <QHash>

#include <QDateTime>
#include <QDebug>
#include <QTimer>

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

    QElapsedTimer lastTcpActivity; //to keep last tcp action on server, users may work over udp for hours but tcp becomes free so OS closes idle tcp sockets, so we dont like this


    bool start(quint16 port,
               quint16 udpPort);

    UserModel* loginUser(
        const LoginRequestPacket &req,
        QTcpSocket* socket,
        QString &errorMessage);

    UserModel* findUser(
        QTcpSocket* socket);

    void removeUser(UserModel* user);

    void removeSession(QTcpSocket *socket);
    void disconnectUser(UserModel *user, bool connectionLost=false);

    Channel* createChannel(
        const QString& name,
        const QString& password, ChannelType::Type type,
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
        const QString& password,
        ChatMessageChunkPacket& chunkResult);

    QList<UserModel*> users() const;
    QList<UserModel *> allUsers() const;

    QByteArray buildServerState();


    //prints
    void printChannels();
    void printChannelWithUsersIn();
    void printUsers(); //online/connected
    void printAllUsers();


    Channel *updateChannel(quint64 channelId, const QString &name, const QString &pass, bool saveChats);
    bool updateChannel(Channel* channel, const QString &name, const QString &pass, bool saveChats);
    bool deleteChannel(Channel *channel);

    Channel *findChannelById(quint64 id);
    UserModel *findUser(quint64 userId); //among connected Users not all users!!!!
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
    UserModel *findUserByIdentity(const QString &identity);


    bool updateUserActivityStatus(UserModel *user, const QString &newStatus);
    Database *db() const;

    QString uploadsDirectory() const;
    bool joinTextChannel(UserModel *user, quint64 channelId, const QString &password, BeanChatCommon::ChatMessageChunkPacket &chunkResult);
private slots:
    void onNewConnection();


private:
    ServerInfo m_info;
    const QString m_uploadsDirectoryName = "uploads";
    quint64 m_nextUserId = 1;

    /*
     every e.g 2min check user's last TCP activity if it exceed send him a IsEverythingOk to user and wait for his
    response YesEverythingIsOk to keep TCP alive (idle tcp sockets are closed by os like if it exceed 5min)
    */
    QTimer m_keepIsTcpAlive;

    Database* m_db=nullptr;
    QTcpServer m_server;

    QHash<QTcpSocket*, ClientSession*> m_sessions;

    QList<UserModel*> m_users;
    QList<UserModel*> m_allUsers;  // all registered users

    QList<Channel*> m_channels;

    UdpServer* m_udpServer;
};
