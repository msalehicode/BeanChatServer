#pragma once

#include <QObject>
#include <QSqlDatabase>
#include "channel.h"

class Database
{
public:
    Database();

    bool open();

    bool createTables();

    //channel
    bool createChannel(Channel* channel);
    bool updateChannel(Channel* channel);
    bool deleteChannel(quint64 id);
    QList<Channel*> loadChannels();

private:
    QSqlDatabase m_db;
};
