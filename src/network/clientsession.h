#pragma once

#include <QObject>
#include <QTcpSocket>

#include "../network/packet.h"

class Server;
class UserModel;

class ClientSession : public QObject
{
    Q_OBJECT

public:
    explicit ClientSession(
        QTcpSocket* socket,
        Server* server);

    UserModel* user() const;


    void sendToChannel(PacketType type, const QByteArray &payload);
    void sentToChannelExceptSender(PacketType type, const QByteArray &payload);
    void sendToSender(PacketType type, const QByteArray& payload);
    void sendToEveryone(PacketType type, const QByteArray& payload);
    void sendToEveryoneExceptSender(PacketType type, const QByteArray& payload);

    void forceDisconnect(bool connectionLost);
private slots:
    void onReadyRead();
    void onDisconnected();

private:
    void processPacket(
        const Packet& packet);

    void handleLogin(
        const QByteArray& payload);

private:
    QByteArray m_buffer;

    QTcpSocket* m_socket;
    Server* m_server;

    UserModel* m_user = nullptr;
};
