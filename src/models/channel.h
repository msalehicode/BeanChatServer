#pragma once

#include <QString>
#include <QList>
#include <QDateTime>

class UserModel;

class Channel
{
public:

    quint64 id = 0;

    QString name;

    QString password;

    UserModel* owner=nullptr;
    QString ownerIdentity;
    int displayOrder = 0;
    QDateTime createdAt;
    QDateTime updatedAt;

    bool saveChats = false;

    QList<UserModel*> users;
};
