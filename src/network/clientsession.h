#pragma once

#include <QObject>
#include <QTcpSocket>

#include "../network/packet.h"

class Server;
class User;

class ClientSession : public QObject
{
    Q_OBJECT

public:
    explicit ClientSession(
        QTcpSocket* socket,
        Server* server);

    User* user() const;

    void sendPacket(
        PacketType type,
        const QByteArray& payload);

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

    User* m_user = nullptr;
};
