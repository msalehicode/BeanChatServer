#pragma once

#include <QString>
#include <QDataStream>

enum class PacketType : quint16
{
    Invalid = 0,

    LoginRequest = 1,
    LoginResponse = 2,

    UserConnected = 3,
    UserDisconnected = 4,

    CreateChannel = 5,
    JoinChannel = 6,

    ChannelCreated = 7,
    UserJoinedChannel = 8,

    ChatMessage = 9,

    UserMuted = 10,
    UserUnmuted = 11,

    UserDeafened = 12,
    UserUndeafened = 13,

    UserMoved = 14,

    ChannelDeleted = 15,

    VoiceData = 16,

    VideoData = 17,

    PingRequest = 18,
    PingResponse = 19,

    RequestServerState = 20,
    ServerState = 21
};

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
    bool accepted;
    QString message;
};

inline QDataStream&
operator<<(QDataStream& out,
           const LoginResponsePacket& p)
{
    out << p.accepted
        << p.message;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           LoginResponsePacket& p)
{
    in >> p.accepted
        >> p.message;

    return in;
}


struct UserConnectedPacket
{
    quint64 id;
    QString username;
};

inline QDataStream&
operator<<(QDataStream& out,
           const UserConnectedPacket& p)
{
    out << p.id
        << p.username;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           UserConnectedPacket& p)
{
    in >> p.id
        >> p.username;

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
    quint64 channelId;
};

inline QDataStream&
operator<<(QDataStream& out,
           const UserJoinedChannelPacket& p)
{
    out << p.userId
        << p.channelId;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           UserJoinedChannelPacket& p)
{
    in >> p.userId
        >> p.channelId;

    return in;
}

struct ChatMessagePacket
{
    quint64 senderId;

    quint64 channelId;

    QString text;
};

inline QDataStream&
operator<<(QDataStream& out,
           const ChatMessagePacket& p)
{
    out << p.senderId
        << p.channelId
        << p.text;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           ChatMessagePacket& p)
{
    in >> p.senderId
        >> p.channelId
        >> p.text;

    return in;
}


struct SendMessagePacket
{
    QString text;
};

inline QDataStream&
operator<<(QDataStream& out,
           const SendMessagePacket& p)
{
    out << p.text;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           SendMessagePacket& p)
{
    in >> p.text;

    return in;
}


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

    bool muted;
    bool deafened;
};

inline QDataStream&
operator<<(QDataStream& out,
           const UserInfo& p)
{
    out << p.id
        << p.username
        << p.channelId
        << p.muted
        << p.deafened;

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
        >> p.deafened;

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

