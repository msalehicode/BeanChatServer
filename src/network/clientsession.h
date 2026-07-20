#pragma once

#include <QObject>
#include <QTcpSocket>

//from BeanChatCommon
#include <protocol/Packet.h>
#include <protocol/Packets.h>
#include <protocol/PacketHelpers.h>
#include <protocol/ProtocolLimits.h>
#include <protocol/ProtocolVersion.h>
#include <crypto/Crypto.h>
using namespace BeanChatCommon;


#include "../server/server.h"
#include "../models/user.h"
#include "../server/uploadsession.h"

#include <QDebug>
#include <QUuid>
#include <QFile>

#include <QVersionNumber>
#include <QCryptographicHash>

class Server;
class UserModel;

class ClientSession : public QObject
{
    Q_OBJECT

public:
    explicit ClientSession(
        QTcpSocket* socket,
        Server* server);

    ~ClientSession();

    UserModel* user() const;


    void sendToChannel(PacketType type, const QByteArray &payload);
    void sentToChannelExceptSender(PacketType type, const QByteArray &payload);
    void sendToSender(PacketType type, const QByteArray& payload);
    void sendToEveryone(PacketType type, const QByteArray& payload);
    void sendToEveryoneExceptSender(PacketType type, const QByteArray& payload);

    void forceDisconnect(bool connectionLost);

    QString formatRemainingTime(qint64 seconds);
    quint64 saveMessage(quint64 channelId, const SendMessagePacket &msg);
private slots:
    void onReadyRead();
    void onDisconnected();

private:
    void processPacket(
        const Packet& packet);

    void handleLoginProof(const QByteArray &payload);
    void handleLogin(
        const QByteArray& payload);

private:
    QByteArray m_buffer;
    bool m_connectionLost=false;

    //login stuff
    LoginRequestPacket m_pendingLogin;
    QByteArray m_pendingChallenge;

    //uploads
    QHash<quint64, UploadSession*> m_uploads;
    quint64 m_nextUploadId = 1;
    quint64 nextUploadId();
    void destroyUpload(
        QHash<quint64, UploadSession*>::iterator it,
        bool removeFile);

    QTcpSocket* m_socket;
    Server* m_server;

    UserModel* m_user = nullptr;
};
