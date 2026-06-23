#pragma once

#include <QString>
#include <QList>

class UserModel;

class Channel
{
public:

    quint64 id = 0;

    QString name;

    QString password;

    bool permanentChat = false;
    bool temporaryChat = true;

    QList<UserModel*> users;
};
