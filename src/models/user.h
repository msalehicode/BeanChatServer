#pragma once

#include <QString>
#include <QTcpSocket>

class Channel;

class UserModel
{
public:

    quint64 id = 0;

    QString username;
    QString identity;

    QHostAddress udpAddress;
    quint16 udpPort = 0;

    bool udpRegistered = false;

    QString ip;
    quint16 port;

    qint64 connectedSince = 0;

    bool muted = false;
    bool deafened = false;
    bool camera = false;

    int ping = 0;
    float packetLoss = 0.0f;

    QTcpSocket* socket = nullptr;

    Channel* currentChannel = nullptr;




    // Client information
    QString appVersion;      // "1.2.5"
    QString buildType;       // "Release", "Debug"

    // Operating system
    QString osName;          // "Windows", "Linux", "macOS"
    QString osVersion;       // "11", "Ubuntu 24.04"

    // Hardware / machine
    QString machineName;     // Computer hostname
    QString machineId;       // Stable generated ID
};
