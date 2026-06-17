#pragma once

#include <QObject>

#include <QTcpServer>
#include <QTcpSocket>

#include <QList>

// class User;
// class Channel;
#include "../models/user.h"
#include "../models/channel.h"

class Server : public QObject
{
    Q_OBJECT

public:
    explicit Server(
        QObject* parent = nullptr);

    bool start(
        quint16 port);

    User* loginUser(
        const QString& username,
        const QString& identity,
        QTcpSocket* socket);

    void removeUser(
        User* user);

    Channel* createChannel(
        const QString& name,
        const QString& password,
        bool permanentChat,
        bool temporaryChat);

    void saveMessage(
        quint64 channelId,
        quint64 senderId,
        const QString& text);

    void broadcastMessage(
        User* sender,
        const QString& text);


    void notifyEveryone(const QString& text);

    bool joinChannel(
        User* user,
        quint64 channelId,
        const QString& password);


    void printChannels()
    {
        qDebug() << "channels:";
        for(Channel* channel: m_channels)
        {
            qDebug() << channel->id << " " << channel->name << " " << channel->password;
        }
    }

    void printChannelWithUsersIn()
    {
        qDebug() << "channels:";
        for(Channel* channel: m_channels)
        {
            qDebug() << channel->id << " " << channel->name << " " << channel->password;
            for(User* c : channel->users)
                qDebug() << "     " << c->username;
        }
    }

    void printUsers()
    {
        qDebug() << "users:";
        for(User* user: m_users)
        {
            qDebug() << user->id << " " << user->username << " " << user->identity << " " << user->ip;
        }
    }
private slots:
    void onNewConnection();

private:
    quint64 m_nextUserId = 1;

    QTcpServer m_server;

    QList<User*> m_users;

    QList<Channel*> m_channels;
};
