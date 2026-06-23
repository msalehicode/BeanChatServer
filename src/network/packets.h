#pragma once

#include <QString>
#include <QDataStream>
#include <QDateTime>

enum class PacketType : quint16
{
    Invalid = 0,

    LoginRequest,
    LoginResponse,

    UserConnected,
    UserDisconnected,

    CreateChannel,
    JoinChannel,

    ChannelCreated,
    UserJoinedChannel,

    ChatMessage,

    UserMuted ,
    UserUnmuted ,

    UserDeafened ,
    UserUndeafened ,

    UserCameraOpened,
    UserCameraClosed,

    UserMoved,

    ChannelDeleted,

    VoiceDataOld,

    VideoData,

    PingRequest,
    PingResponse,

    RequestServerState,
    ServerState,


    UdpRegister = 100,
    VoiceData = 101
};

struct UserStatusChangedPacket
{
    quint64 userId;
    quint64 userChannelId;
    bool status; //status type would set by PacketType::UserMuted or ..
};

inline QDataStream&
operator<<(QDataStream& out,
           const UserStatusChangedPacket& p)
{
    out << p.userId
        << p.userChannelId
        << p.status;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           UserStatusChangedPacket& p)
{
    in >> p.userId
        >> p.userChannelId
        >> p.status;

    return in;
}





struct LoginRequestPacket
{
    QString username;
    QString identity;
};

inline QDataStream&
operator<<(QDataStream& out,
           const LoginRequestPacket& p)
{
    out << p.username
        << p.identity;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           LoginRequestPacket& p)
{
    in >> p.username
        >> p.identity;

    return in;
}


struct LoginResponsePacket
{
    int id;
    bool accepted;
    QString message;
};

inline QDataStream&
operator<<(QDataStream& out,
           const LoginResponsePacket& p)
{
    out << p.id
        << p.accepted
        << p.message;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           LoginResponsePacket& p)
{
    in >> p.id
        >> p.accepted
        >> p.message;

    return in;
}


struct UserConnectedPacket
{
    quint64 id;
    QString username;
    bool muted=false;
    bool deafened=false;
    bool camera=false;
};

inline QDataStream&
operator<<(QDataStream& out,
           const UserConnectedPacket& p)
{
    out << p.id
        << p.username
        << p.muted
        << p.deafened
        << p.camera;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           UserConnectedPacket& p)
{
    in >> p.id
        >> p.username
        >> p.muted
        >> p.deafened
        >> p.camera;

    return in;
}

struct UserDisconnectedPacket
{
    quint64 id;
};

inline QDataStream&
operator<<(QDataStream& out,
           const UserDisconnectedPacket& p)
{
    out << p.id;
    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           UserDisconnectedPacket& p)
{
    in >> p.id;
    return in;
}


struct CreateChannelPacket
{
    QString name;
    QString password;

    bool permanentChat;
    bool temporaryChat;
};

inline QDataStream&
operator<<(QDataStream& out,
           const CreateChannelPacket& p)
{
    out << p.name
        << p.password
        << p.permanentChat
        << p.temporaryChat;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           CreateChannelPacket& p)
{
    in >> p.name
        >> p.password
        >> p.permanentChat
        >> p.temporaryChat;

    return in;
}


struct JoinChannelPacket
{
    quint64 channelId;
    QString password;
};

inline QDataStream&
operator<<(QDataStream& out,
           const JoinChannelPacket& p)
{
    out << p.channelId
        << p.password;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           JoinChannelPacket& p)
{
    in >> p.channelId
        >> p.password;

    return in;
}

struct ChannelCreatedPacket
{
    quint64 id;
    QString name;
    bool isLocked=false;
};

inline QDataStream&
operator<<(QDataStream& out,
           const ChannelCreatedPacket& p)
{
    out << p.id
        << p.name;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           ChannelCreatedPacket& p)
{
    in >> p.id
        >> p.name;

    return in;
}

struct UserJoinedChannelPacket
{
    quint64 userId;
    quint64 channelId=-1;
    quint64 oldChannelId;
};

inline QDataStream&
operator<<(QDataStream& out,
           const UserJoinedChannelPacket& p)
{
    out << p.userId
        << p.channelId
        << p.oldChannelId;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           UserJoinedChannelPacket& p)
{
    in >> p.userId
        >> p.channelId
        >> p.oldChannelId;

    return in;
}

// struct ChatMessagePacket
// {
//     quint64 senderId;

//     quint64 channelId;

//     QString text;
// };

// inline QDataStream&
// operator<<(QDataStream& out,
//            const ChatMessagePacket& p)
// {
//     out << p.senderId
//         << p.channelId
//         << p.text;

//     return out;
// }

// inline QDataStream&
// operator>>(QDataStream& in,
//            ChatMessagePacket& p)
// {
//     in >> p.senderId
//         >> p.channelId
//         >> p.text;

//     return in;
// }


// struct SendMessagePacket
// {
//     QString text;
// };

// inline QDataStream&
// operator<<(QDataStream& out,
//            const SendMessagePacket& p)
// {
//     out << p.text;

//     return out;
// }

// inline QDataStream&
// operator>>(QDataStream& in,
//            SendMessagePacket& p)
// {
//     in >> p.text;

//     return in;
// }


struct ChannelInfo
{
    quint64 id;
    QString name;
    bool isLocked;
    bool permanentChat;
    bool temporaryChat;
};

inline QDataStream&
operator<<(QDataStream& out,
           const ChannelInfo& p)
{
    out << p.id
        << p.name
        << p.permanentChat
        << p.temporaryChat
        << p.isLocked;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           ChannelInfo& p)
{
    in >> p.id
        >> p.name
        >> p.permanentChat
        >> p.temporaryChat
        >> p.isLocked;

    return in;
}


struct UserInfo
{
    quint64 id;

    QString username;

    quint64 channelId;

    bool muted=false;
    bool deafened=false;
    bool camera=false;
};

inline QDataStream&
operator<<(QDataStream& out,
           const UserInfo& p)
{
    out << p.id
        << p.username
        << p.channelId
        << p.muted
        << p.deafened
        << p.camera;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           UserInfo& p)
{
    in >> p.id
        >> p.username
        >> p.channelId
        >> p.muted
        >> p.deafened
        >> p.camera;

    return in;
}


struct ServerStatePacket
{
    QList<ChannelInfo> channels;

    QList<UserInfo> users;
};


inline QDataStream&
operator<<(QDataStream& out,
           const ServerStatePacket& p)
{
    out << p.channels
        << p.users;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           ServerStatePacket& p)
{
    in >> p.channels
        >> p.users;

    return in;
}



struct SendMessagePacket
{
    enum Type
    {
        Text,
        Image,
        Video,
        File,
        Link,
        Emoji
    };

    QString text;
    Type type;
    QString mediaPath;
};

inline QDataStream&
operator<<(QDataStream& out,
           const SendMessagePacket& p)
{
    out << p.text
        << p.type
        << p.mediaPath;


    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           SendMessagePacket& p)
{
    in  >> p.text
        >> p.type
        >> p.mediaPath;

    return in;
}





struct ChatMessagePacket
{
    enum Type
    {
        Text,
        Image,
        Video,
        File,
        Link,
        Emoji
    };

    //fill by server
    quint64 messageId;
    quint64 senderId;
    // quint64 channelId;
    QDateTime timestamp=QDateTime::currentDateTime();


    //fill by client
    QString text="";
    Type type = Text;
    QString mediaPath="";
};

inline QDataStream&
operator<<(QDataStream& out,
           const ChatMessagePacket& p)
{
    out << p.messageId
        << p.senderId
        // << p.channelId
        << p.text
        << p.type
        << p.mediaPath
        << p.timestamp;


    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           ChatMessagePacket& p)
{
    in >> p.messageId
        >> p.senderId
        // >> p.channelId
        >> p.text
        >> p.type
        >> p.mediaPath
        >> p.timestamp;

    return in;
}
