#pragma once

#include <QObject>

#include <QTcpServer>
#include <QTcpSocket>

#include <QList>
#include <QHash>

#include "../models/user.h"
#include "../models/channel.h"
#include "../network/packets.h"
#include "../network/udpserver.h"

class ClientSession;

class Server : public QObject
{
    Q_OBJECT

public:
    explicit Server(QObject* parent = nullptr);

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
        UserModel *owner);

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

    void printChannels();

    void printChannelWithUsersIn();

    void printUsers();
private slots:
    void onNewConnection();


private:
    quint64 m_nextUserId = 1;

    QTcpServer m_server;

    QHash<QTcpSocket*, ClientSession*> m_sessions;

    QList<UserModel*> m_users;

    QList<Channel*> m_channels;

    UdpServer* m_udpServer;
};
