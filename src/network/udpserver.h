#pragma once

#include <QObject>
#include <QUdpSocket>

class Server;
class User;

class UdpServer : public QObject
{
    Q_OBJECT

public:
    explicit UdpServer(
        Server* server,
        QObject* parent = nullptr);

    bool start(quint16 port);

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

    User* findUser(
        quint64 userId);

private:
    QUdpSocket m_socket;
    Server* m_server = nullptr;
};
