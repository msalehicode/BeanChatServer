#pragma once

#include <QString>
#include <QTcpSocket>

class Channel;

class User
{
public:

    quint64 id = 0;

    QString username;
    QString identity;

    QString ip;

    qint64 connectedSince = 0;

    bool muted = false;
    bool deafened = false;

    int ping = 0;
    float packetLoss = 0.0f;

    QTcpSocket* socket = nullptr;

    Channel* currentChannel = nullptr;
};
