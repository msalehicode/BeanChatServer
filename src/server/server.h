#pragma once

#include <QObject>

#include <QTcpServer>
#include <QTcpSocket>

#include <QList>

// class User;
// class Channel;
#include "../models/user.h"
#include "../models/channel.h"

#include "../network/packets.h"

#include "../network/udpserver.h"

class Server : public QObject
{
    Q_OBJECT

public:
    explicit Server(
        QObject* parent = nullptr);

    bool start(
        quint16 port, quint16 udpPort);

    UserModel* loginUser(
        const LoginRequestPacket &req,
        QTcpSocket* socket);

    UserModel* findUser(QTcpSocket* socket);

    void removeUser(UserModel* user, bool connectionLost=false);

    Channel* createChannel(
        const QString& name,
        const QString& password,
        bool permanentChat,
        bool temporaryChat);

    void changeUserStatus(PacketType type, QTcpSocket* socket);

    void saveMessage(
        quint64 channelId,
        quint64 senderId,
        const QString& text);

    void broadcastMessage(
        UserModel* sender,
        const QString& text);

    void broadcastMessage(UserModel* sender, SendMessagePacket &message);

    void sendToAll(PacketType pt, const QByteArray& packedData,
                   UserModel* exceptThis=nullptr, QByteArray exceptData=0);
    void sendToUser(UserModel* receiver, const QByteArray& packedData);

    void notifyEveryone(const QString& text);

    bool joinChannel(
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

    QList<UserModel*> m_users;

    QList<Channel*> m_channels;

    UdpServer* m_udpServer;
};
