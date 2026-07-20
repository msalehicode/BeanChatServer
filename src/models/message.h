#pragma once

#include <QString>
#include <QDateTime>
#include <protocol/commonTypes.h>
#include <protocol/packets/MessagePackets.h>

struct Message
{
    quint64 id = 0;

    quint64 channelId = 0;
    quint64 senderId = 0;

    BeanChatCommon::Msg::Type type;

    QString text;

    quint64 attachmentId = 0;

    bool edited = false;
    bool deleted = false;

    QDateTime createdAt;
    QDateTime updatedAt;


    static BeanChatCommon::ChatMessagePacket toPacket(const Message &msg,
                               const QString &senderName)
    {
        BeanChatCommon::ChatMessagePacket p;

        p.messageId   = msg.id;
        p.senderId    = msg.senderId;
        p.senderName  = senderName;
        p.channelId   = msg.channelId;
        p.attachmentId= msg.attachmentId;
        p.timestamp   = msg.createdAt;

        p.text = msg.text;
        p.type = msg.type;

        return p;
    }
};
