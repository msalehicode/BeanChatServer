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

    UserModel* owner=nullptr;

    bool saveChats = false;

    QList<UserModel*> users;
};
