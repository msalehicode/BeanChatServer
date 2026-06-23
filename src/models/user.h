#pragma once

#include <QString>
#include <QTcpSocket>

#include <QHash>
class Channel;


struct PacketLossStats
{
    quint64 windowReceived = 0;
    quint64 windowLost = 0;

    float packetLoss = 0.0f;

    quint32 highestSequence = 0;
};

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

    //ping system over udp
    int ping = 0;
    float packetLoss = 0.0f;
    quint32 nextPingSequence = 0;
    quint64 pingsSent = 0;
    quint64 pongsReceived = 0;
    QHash<quint32, qint64> pendingPings;


    //packetloss
    PacketLossStats voicePacketLossStats;
    PacketLossStats videoPacketLossStats;


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
