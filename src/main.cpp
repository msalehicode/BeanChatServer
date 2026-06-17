#include <QCoreApplication>

#include "models/database.h"
#include "server/server.h"

int main(
    int argc,
    char *argv[])
{
    QCoreApplication app(
        argc,
        argv);

    Database database;

    if(!database.open())
    {
        qCritical()
        << "Failed to open database";

        return -1;
    }

    Server server;

    if(!server.start(9987))
    {
        qCritical()
        << "Failed to start server";

        return -1;
    }

    qInfo()
        << "Server running";

    return app.exec();
}
