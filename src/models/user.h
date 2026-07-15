#pragma once

#include <QString>
#include <QTcpSocket>

#include <QHash>
#include <QElapsedTimer>
#include <QDateTime>

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

    //info from database
    quint64 id = 0;
    QString username;
    QString identity;
    QString avatarHash; //store generated hash for each avatar
    quint64 totalConnected=0;
    QDateTime firstLogin;

    //
    Channel* currentChannel = nullptr;
    qint64 connectedSince = 0;

    //permissions (grant/denied)
    bool canTalk=true;
    bool canChat=true;
    bool canShareVideo=true;
    bool canCreateChannel=true;
    bool isAdmin=false;
    bool banned=false;
    quint64 banExpiresAt=0; //ban seconds
    QString banReason;


    //user local temp status/info, (actions like mute my mic, and ..)
    bool muted = false;
    bool deafened = false;
    bool camera = false;
    QString appVersion;      // "1.2.5"
    QString buildType;       // "Release", "Debug", "Beta", "Alpha" , ...
    QString osName;          // "Windows", "Linux", "macOS"
    QString osVersion;       // "11", "Ubuntu 24.04"
    QString machineName;     // Computer hostname
    QString machineId;       // Stable generated ID


    //connection TCP
    QString ip;
    quint16 port;
    QTcpSocket* socket = nullptr;


    //connection UDP
    QHostAddress udpAddress;
    quint16 udpPort = 0;
    bool udpRegistered = false;

    //ping system over udp
    int ping = 0;
    float packetLoss = 0.0f;
    quint32 nextPingSequence = 0;
    quint64 pingsSent = 0;
    quint64 pongsReceived = 0;
    QHash<quint32, qint64> pendingPings;
    QElapsedTimer lastUdpActivity; //to know when user didn't response for a while that connection is lost.

    //packetloss udp
    PacketLossStats voicePacketLossStats;
    PacketLossStats videoPacketLossStats;
};
