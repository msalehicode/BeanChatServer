#pragma once

#include <QString>
#include <QDataStream>
#include <QDateTime>
#include <limits>


//when user don't find server's avatarHash \
    would send RequestAvatars and if inside that list userId was equal to this \
    server return this code as userId and fill avatarHash, avatarData as response.
constexpr quint64 RESERVED_TO_ASK_SERVERS_AVATAR =
    std::numeric_limits<quint64>::max()-1;

enum class PacketType : quint16
{
    Invalid = 0,

    //login
    LoginRequest,
    LoginResponse,


    //connection
    UserConnected,
    UserDisconnected,
    UserConnectionLost,


    //channel
    CreateChannel,
    UpdateChannel,
    DeleteChannel,

    ChannelCreated,
    ChannelUpdated,
    ChannelDeleted,

    ChatMessage,



    //join,move
    JoinChannel,
    UserJoinedChannel,

    MoveUser,
    UserMoved,


    //status
    UserMuted ,
    UserUnmuted ,

    UserDeafened ,
    UserUndeafened ,

    UserCameraOpened,
    UserCameraClosed,


    //user info
    RequestAvatars, //user sends those ids has no cached matches for those avatars
    ResponseAvatars, //server send avatar data for those ids

    UpdateUserInfo, //user sends to ask to update username, avatar, ...
    UserInfoChanged, //users receive this when someone changed his info

    //state
    RequestServerState,
    ServerState, //response to RequestServerState


    //udp codes:
    UdpLoginRequest = 100, //when user loginResponse arrived client sends this to server to register udp socket.
    UdpLoginResponse = 101, //response to Udplogin
    UdpPingRequest = 102, //continuesly server sends to registered UDP clients
    UdpPingResponse = 103, //client responses to server's PingRequest
    UdpVoiceData = 104,
    UdpVideoData =105
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



struct MoveUserPacket
{
    quint64 userId;
    quint64 channelId;
    QString channelPassword="";
};

inline QDataStream&
operator<<(QDataStream& out,
           const MoveUserPacket& p)
{
    out << p.userId
        << p.channelId
        << p.channelPassword;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           MoveUserPacket& p)
{
    in >> p.userId
       >> p.channelId
       >> p.channelPassword;

    return in;
}



struct LoginRequestPacket
{
    QString username;
    QString identity;

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

inline QDataStream&
operator<<(QDataStream& out,
           const LoginRequestPacket& p)
{
    out << p.username
        << p.identity
        << p.appVersion
        << p.buildType
        << p.osName
        << p.osVersion
        << p.machineName
        << p.machineId;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           LoginRequestPacket& p)
{
    in >> p.username
        >> p.identity
        >> p.appVersion
        >> p.buildType
        >> p.osName
        >> p.osVersion
        >> p.machineName
        >> p.machineId;

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
    QString avatarHash;
    bool muted=false;
    bool deafened=false;
    bool camera=false;

    // Client information
    QString appVersion;      // "1.2.5"
    QString buildType;       // "Release", "Debug"

    // Operating system
    QString osName;          // "Windows", "Linux", "macOS"
    QString osVersion;       // "11", "Ubuntu 24.04"

};

inline QDataStream&
operator<<(QDataStream& out,
           const UserConnectedPacket& p)
{
    out << p.id
        << p.username
        << p.avatarHash
        << p.muted
        << p.deafened
        << p.camera
        << p.appVersion
        << p.buildType
        << p.osName
        << p.osVersion;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           UserConnectedPacket& p)
{
    in >> p.id
        >> p.username
        >> p.avatarHash
        >> p.muted
        >> p.deafened
        >> p.camera
        >> p.appVersion
        >> p.buildType
        >> p.osName
        >> p.osVersion;

    return in;
}

struct UserDisconnectedPacket
{
    quint64 id;
    bool wasConnectionLost=false;
};

inline QDataStream&
operator<<(QDataStream& out,
           const UserDisconnectedPacket& p)
{
    out << p.id
        << p.wasConnectionLost;
    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           UserDisconnectedPacket& p)
{
    in >> p.id
       >> p.wasConnectionLost;
    return in;
}



struct CreateChannelPacket
{
    QString name;
    QString password;

    bool saveChats;
};

inline QDataStream&
operator<<(QDataStream& out,
           const CreateChannelPacket& p)
{
    out << p.name
        << p.password
        << p.saveChats;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           CreateChannelPacket& p)
{
    in >> p.name
        >> p.password
        >> p.saveChats;

    return in;
}


struct UpdateChannelPacket
{
    quint64 channelId;
    QString name;
    QString password;
    bool saveChats;
};

inline QDataStream&
operator<<(QDataStream& out,
           const UpdateChannelPacket& p)
{
    out << p.channelId
        << p.name
        << p.password
        << p.saveChats;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           UpdateChannelPacket& p)
{
    in  >> p.channelId
        >> p.name
        >> p.password
        >> p.saveChats;

    return in;
}




struct ChannelUpdatedPacket
{
    quint64 channelId;
    QString name;
    bool isLocked;
    bool saveChats;
};

inline QDataStream&
operator<<(QDataStream& out,
           const ChannelUpdatedPacket& p)
{
    out << p.channelId
        << p.name
        << p.isLocked
        << p.saveChats;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           ChannelUpdatedPacket& p)
{
    in  >> p.channelId
        >> p.name
        >> p.isLocked
        >> p.saveChats;

    return in;
}

struct DeleteChannelPacket
{
    quint64 channelId;
};

inline QDataStream&
operator<<(QDataStream& out,
           const DeleteChannelPacket& p)
{
    out << p.channelId;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           DeleteChannelPacket& p)
{
    in  >> p.channelId;

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
    bool saveChats=false;
};

inline QDataStream&
operator<<(QDataStream& out,
           const ChannelCreatedPacket& p)
{
    out << p.id
        << p.name
        << p.isLocked
        << p.saveChats;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           ChannelCreatedPacket& p)
{
    in >> p.id
        >> p.name
        >> p.isLocked
        >> p.saveChats;

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
    bool saveChats;
};

inline QDataStream&
operator<<(QDataStream& out,
           const ChannelInfo& p)
{
    out << p.id
        << p.name
        << p.saveChats
        << p.isLocked;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           ChannelInfo& p)
{
    in >> p.id
        >> p.name
        >> p.saveChats
        >> p.isLocked;

    return in;
}


struct UserInfo
{
    quint64 id;

    QString username;
    QString avatarHash;

    quint64 channelId;

    bool muted=false;
    bool deafened=false;
    bool camera=false;


    // Client information
    QString appVersion;      // "1.2.5"
    QString buildType;       // "Release", "Debug"

    // Operating system
    QString osName;          // "Windows", "Linux", "macOS"
    QString osVersion;       // "11", "Ubuntu 24.04"
};

inline QDataStream&
operator<<(QDataStream& out,
           const UserInfo& p)
{
    out << p.id
        << p.username
        << p.avatarHash
        << p.channelId
        << p.muted
        << p.deafened
        << p.camera
        << p.appVersion
        << p.buildType
        << p.osName
        << p.osVersion;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           UserInfo& p)
{
    in >> p.id
        >> p.username
        >> p.avatarHash
        >> p.channelId
        >> p.muted
        >> p.deafened
        >> p.camera
        >> p.appVersion
        >> p.buildType
        >> p.osName
        >> p.osVersion;


    return in;
}

struct ServerInfo
{
    QString name;
    QString version;
    QString website;
    QDateTime startTime;

    //store server's avatarHash, if user couldn't find that hash in cached acatars would ask for avatar.
    //and would store inside user's servers directory
    QString avatarHash;
    QString oldAvatarHash; //when avatar changed to tell users delete old avatar

    bool operator==(const ServerInfo &other) const
    {
        return name == other.name &&
               version == other.version &&
               website == other.website &&
               startTime == other.startTime &&
               avatarHash == other.avatarHash &&
               oldAvatarHash == other.oldAvatarHash;
    }

    bool operator!=(const ServerInfo &other) const
    {
        return !(*this == other);
    }
};


inline QDataStream& operator<<(QDataStream& out,
                               const ServerInfo& p)
{
    out << p.name
        << p.version
        << p.website
        << p.avatarHash
        << p.oldAvatarHash
        << p.startTime;

    return out;
}

inline QDataStream& operator>>(QDataStream& in,
                               ServerInfo& p)
{
     in >> p.name
        >> p.version
        >> p.website
        >> p.avatarHash
        >> p.oldAvatarHash
        >> p.startTime;

    return in;
}


struct ServerStatePacket
{
    ServerInfo serverInfo;
    QList<ChannelInfo> channels;
    QList<UserInfo> users;
};


inline QDataStream&
operator<<(QDataStream& out,
           const ServerStatePacket& p)
{
    out << p.serverInfo
        << p.channels
        << p.users;

    return out;
}

inline QDataStream&
operator>>(QDataStream& in,
           ServerStatePacket& p)
{
    in  >> p.serverInfo
        >> p.channels
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
    QString senderName; //sometimes user has disconnected, we don't access to his id to findout what was his name
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
        << p.senderName
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
        >> p.senderName
        >> p.text
        >> p.type
        >> p.mediaPath
        >> p.timestamp;

    return in;
}


struct PingPacket
{
    quint32 sequence;
    int lastPing=-1;
    float voicePacketLoss=0.0f;
    float videoPacketLoss=0.0f;
};

inline QDataStream& operator<<(QDataStream& out,
                               const PingPacket& p)
{
    out << p.sequence
        << p.lastPing
        << p.voicePacketLoss
        << p.videoPacketLoss;

    return out;
}

inline QDataStream& operator>>(QDataStream& in,
                               PingPacket& p)
{
    in >> p.sequence
       >> p.lastPing
       >> p.voicePacketLoss
       >> p.videoPacketLoss;

    return in;
}












struct RequestAvatarsPacket
{
    QList<quint64> notFoundIds; //list of userId
};

inline QDataStream& operator<<(QDataStream& out,
                               const RequestAvatarsPacket& p)
{
    out << p.notFoundIds;
    return out;
}

inline QDataStream& operator>>(QDataStream& in,
                               RequestAvatarsPacket& p)
{
    in >> p.notFoundIds;
    return in;
}


struct UserAvatar
{
    quint64 userId;
    QString avatarHash;
    QString oldHash; //if isn't empty -> to delete old pic in cache user directory
    QByteArray imageData;

    void clear()
    {
        userId=-1;
        avatarHash.clear();
        oldHash.clear();
        imageData.clear();
    }
};
inline QDataStream& operator<<(QDataStream& out,
                               const UserAvatar& p)
{
    out << p.userId
        << p.avatarHash
        << p.oldHash
        << p.imageData;

    return out;
}

inline QDataStream& operator>>(QDataStream& in,
                               UserAvatar& p)
{
    in >> p.userId
        >> p.avatarHash
        >> p.oldHash
        >> p.imageData;

    return in;
}


struct ResponseAvatarsPacket
{
    QList<UserAvatar> avatars; //userId, oldHash, avatarHash, image data
};

inline QDataStream& operator<<(QDataStream& out,
                               const ResponseAvatarsPacket& p)
{
    out << p.avatars;

    return out;
}

inline QDataStream& operator>>(QDataStream& in,
                               ResponseAvatarsPacket& p)
{
    in >> p.avatars;

    return in;
}



enum class UpdateUserInfoType
{
    Username,
    Description,
    Identity,
    Avatar,
    ActivityStatus
};

struct UpdateUserInfoPacket
{
    UpdateUserInfoType updateType;
    QString payloadValue;
    QByteArray paylaodData; //for avatar image.
};

inline QDataStream& operator<<(QDataStream& out,
                               const UpdateUserInfoPacket& p)
{
    out << p.updateType
        << p.payloadValue
        << p.paylaodData;

    return out;
}

inline QDataStream& operator>>(QDataStream& in,
                               UpdateUserInfoPacket& p)
{
    in >> p.updateType
        >> p.payloadValue
        >> p.paylaodData;

    return in;
}



struct UserInfoChangedPacket
{
    quint64 userId;
    UpdateUserInfoType updateType;
    QString payloadValue; //e.g to pass avatar Hash
    QString payloadSecondValue; //for e.g old hash to tell others delete old avatarHash file
    QByteArray payloadData; //e.g: for avatar image data.
};

inline QDataStream& operator<<(QDataStream& out,
                               const UserInfoChangedPacket& p)
{
    out << p.userId
        << p.updateType
        << p.payloadValue
        << p.payloadSecondValue
        << p.payloadData;

    return out;
}

inline QDataStream& operator>>(QDataStream& in,
                               UserInfoChangedPacket& p)
{
    in  >> p.userId
        >> p.updateType
        >> p.payloadValue
        >> p.payloadSecondValue
        >> p.payloadData;

    return in;
}
