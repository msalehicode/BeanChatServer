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


    //get port from paremeters otherwise use default
    bool ok1;
    quint16 port = DEFAULT_PORT;
    if (argc >= 2)
    {
        port = QString(argv[1]).toUShort(&ok1);

        if (!ok1)
        {
            qCritical() << "Usage:" << argv[0] << " <Port for tcp and udp>";
            return -1;
        }
    }


    Database database;

    if(!database.open())
    {
        qCritical() << "Failed to open database";

        return -1;
    }

    Server server;

    if(!server.start(port,port)) //tcp, udp are using the same port number
    {
        qCritical() << "Failed to start server";

        return -1;
    }

    qInfo() << "server version is: " << SERVER_VERSION;
    qInfo() << "Server is running...";

    return app.exec();
}
