#pragma once

#include <QString>
#include <QTcpSocket>

#include <QHash>
#include <QElapsedTimer>
#include <QDateTime>


#include <protocol/commonTypes.h>

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

    // ======================= info from database =======================
    quint64 id = 0;
    QString username;
    QString identity;
    QString avatarHash; //store generated hash for each avatar
    QString oldAvatarHash=""; //to prevent leak user's avatar when someone is offline and one changed his avatar.
    quint64 totalConnected=0;
    QDateTime firstLogin;
    //permissions (grant/denied)
    bool canTalk=true;
    bool canChat=true;
    bool canShareVideo=true;
    bool canCreateChannel=true;
    bool isAdmin=false;
    bool banned=false;
    quint64 banExpiresAt=0; //ban seconds
    QString banReason;


    // =======================  RUN TIME but user must fill these . =======================
    //no need to reset because when  user wants connect to server fill these
    QString appVersion;      // "1.2.5"
    QString buildType;       // "Release", "Debug", "Beta", "Alpha" , ...
    QString osName;          // "Windows", "Linux", "macOS"
    QString osVersion;       // "11", "Ubuntu 24.04"
    QString machineName;     // Computer hostname
    QString machineId;       // Stable generated ID



    // ======================= RUN TIME 2 =======================
     // (need to be reset for every session)
    Channel* currentChannel = nullptr;
    qint64 connectedSince = 0;
    bool connected=false; //when user is disconnected set this
    BeanChatCommon::Presence::Status status= BeanChatCommon::Presence::Status::Unknown;
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
    QElapsedTimer lastUdpActivity; //to know when user didn't response for a while that connection is lost.
    //packetloss udp
    PacketLossStats voicePacketLossStats;
    PacketLossStats videoPacketLossStats;


    // =======================  CONNECTION (SOCKETS) =======================
    //connection TCP
    QString ip;
    quint16 port;
    QTcpSocket* socket = nullptr;

    //connection UDP
    QHostAddress udpAddress;
    quint16 udpPort = 0;
    bool udpRegistered = false;


    void resetSession()
    {

        // reset RUN TIME 2:
        currentChannel = nullptr;
        // connectedSince=0; //if logged in fine would set it
        // status= BeanChatCommon::Presence::Status::Unknown; //user sends this while login, no need to reset
        connected = false;        
        muted = false;
        deafened = false;
        camera = false;

        //reset ping and packetloss system
        ping = 0;
        packetLoss = 0.0f;
        nextPingSequence = 0;
        pingsSent = 0;
        pongsReceived = 0;
        pendingPings.clear();
        voicePacketLossStats = {};
        videoPacketLossStats = {};

        //tcp ip port and socket would be overwrite when user connected to login no need to reset them

        //reset tcp info
        udpRegistered = false;
        udpAddress = {};
        udpPort = 0;
    }
};
