#include <QCoreApplication>
#include <QHostAddress>
#include <QDebug>
#include "server.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    ChatServer server;
    if (server.listen(QHostAddress::Any, 12345)) {
        qDebug() << "Server is running on port 12345";
    } else {
        qDebug() << "Failed to start server:" << server.errorString();
        return 1;
    }
    
    return app.exec();
}