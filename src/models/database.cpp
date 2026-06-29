#include "database.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

Database::Database()
{
}

bool Database::open()
{
    m_db =
        QSqlDatabase::addDatabase(
            "QSQLITE");

    m_db.setDatabaseName(
        DATABASE_FILE_NAME);

    if(!m_db.open())
    {
        qDebug()
        << m_db.lastError();

        return false;
    }

    return createTables();
}

bool Database::createTables()
{
    QSqlQuery query;

    query.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "identity TEXT UNIQUE,"
        "username TEXT"
        ");");

    query.exec(
        "CREATE TABLE IF NOT EXISTS channels ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT,"
        "password TEXT,"
        "permanentChat INTEGER,"
        "temporaryChat INTEGER"
        ");");

    query.exec(
        "CREATE TABLE IF NOT EXISTS messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "channelId INTEGER,"
        "senderId INTEGER,"
        "message TEXT,"
        "createdAt DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");");

    qDebug()
        << "Database initialized";

    return true;
}
