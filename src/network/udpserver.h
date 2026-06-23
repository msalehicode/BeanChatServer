#pragma once

#include <QObject>
#include <QUdpSocket>

#include <QTimer>
#include <QDateTime>

#include "../models/user.h"

class Server;

class UdpServer : public QObject
{
    Q_OBJECT

public:
    explicit UdpServer(Server* server, QObject* parent = nullptr);

    bool start(quint16 port);

    void sendPings();
    void processPong(const QNetworkDatagram& datagram, QDataStream& stream);


    void updatePacketLoss();
    void calculateUpdatePacketLoss(PacketLossStats& stats);
    void calculatePacketLoss(quint32 packetSequence, PacketLossStats& stats);
private slots:
    void onReadyRead();

private:
    void processRegister(
        const QNetworkDatagram& datagram,
        QDataStream& stream);

    void processVoice(
        const QNetworkDatagram& datagram,
        QDataStream& stream);

    void processVideo(
        const QNetworkDatagram& datagram,
        QDataStream& stream);

    UserModel* findUser(
        quint64 userId);

private:
    QUdpSocket m_socket;
    Server* m_server = nullptr;

    QTimer m_pingTimer;

    QTimer m_packetLossTimer;
};
