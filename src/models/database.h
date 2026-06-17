#pragma once

#include <QObject>
#include <QSqlDatabase>

class Database
{
public:
    Database();

    bool open();

    bool createTables();

private:
    QSqlDatabase m_db;
};
