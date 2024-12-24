#include "server.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

ChatServer::ChatServer(QObject *parent) : QTcpServer(parent) {
    // Create default public room
    ChatRoom publicRoom;
    publicRoom.name = "Public";
    chatRooms["Public"] = publicRoom;
}

void ChatServer::incomingConnection(qintptr socketDescriptor) {
    QTcpSocket *clientSocket = new QTcpSocket(this);
    if (clientSocket->setSocketDescriptor(socketDescriptor)) {
        connect(clientSocket, &QTcpSocket::readyRead, this, [this, clientSocket]() {
            processMessage(clientSocket, clientSocket->readAll());
        });

        connect(clientSocket, &QTcpSocket::disconnected, this, [this, clientSocket]() {
            handleDisconnection(clientSocket);
        });

        sendRoomListToClient(clientSocket);
        qDebug() << "New client connected";
    } else {
        clientSocket->deleteLater();
        qDebug() << "Failed to set socket descriptor";
    }
}

void ChatServer::processMessage(QTcpSocket* socket, const QByteArray& data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return;

    QJsonObject msg = doc.object();
    QString type = msg["type"].toString();

    if (type == "register") {
        handleRegistration(socket, msg);
    } else if (type == "login") {
        handleLogin(socket, msg);
    } else if (type == "createRoom") {
        handleCreateRoom(socket, msg);
    } else if (type == "joinRoom") {
        handleJoinRoom(socket, msg);
    } else if (type == "message") {
        handleChatMessage(socket, msg);
    }
}

void ChatServer::handleRegistration(QTcpSocket* socket, const QJsonObject& data) {
    QString username = data["username"].toString();
    QString password = data["password"].toString();

    if (username.isEmpty() || password.isEmpty()) {
        sendError(socket, "Invalid username or password");
        return;
    }

    if (registeredUsers.contains(username)) {
        sendError(socket, "Username already exists");
        return;
    }

    registeredUsers[username] = { username, password, "" };

    sendToClient(socket, { {"type", "registrationSuccess"} });
    qDebug() << "New user registered:" << username;
}

void ChatServer::handleLogin(QTcpSocket* socket, const QJsonObject& data) {
    QString username = data["username"].toString();
    QString password = data["password"].toString();

    if (!registeredUsers.contains(username) || registeredUsers[username].password != password) {
        sendError(socket, "Invalid username or password");
        return;
    }

    activeUsers[socket] = username;
    chatRooms["Public"].participants.insert(socket);

    sendToClient(socket, { {"type", "loginSuccess"} });
    qDebug() << username << "logged in";
}

void ChatServer::handleCreateRoom(QTcpSocket* socket, const QJsonObject& data) {
    if (!activeUsers.contains(socket)) {
        sendError(socket, "You must be logged in");
        return;
    }

    QString roomName = data["room"].toString();
    if (roomName.isEmpty() || chatRooms.contains(roomName)) {
        sendError(socket, "Invalid or duplicate room name");
        return;
    }

    chatRooms[roomName] = { roomName, data["password"].toString() };
    broadcastRoomList();
    qDebug() << "New room created:" << roomName;
}

void ChatServer::handleJoinRoom(QTcpSocket* socket, const QJsonObject& data) {
    if (!activeUsers.contains(socket)) {
        sendError(socket, "You must be logged in");
        return;
    }

    QString roomName = data["room"].toString();
    if (!chatRooms.contains(roomName)) {
        sendError(socket, "Room does not exist");
        return;
    }

    ChatRoom& room = chatRooms[roomName];
    if (!room.password.isEmpty() && room.password != data["password"].toString()) {
        sendError(socket, "Invalid room password");
        return;
    }

    QString username = activeUsers[socket];
    QString currentRoom = registeredUsers[username].currentRoom;
    if (!currentRoom.isEmpty()) {
        chatRooms[currentRoom].participants.remove(socket);
    }

    room.participants.insert(socket);
    registeredUsers[username].currentRoom = roomName;

    broadcastToRoom(roomName, { {"type", "message"}, {"text", username + " has joined the room"} });
    qDebug() << username << "joined room:" << roomName;
}

void ChatServer::handleChatMessage(QTcpSocket* socket, const QJsonObject& data) {
    if (!activeUsers.contains(socket)) {
        sendError(socket, "You must be logged in");
        return;
    }

    QString text = data["text"].toString();
    if (text.isEmpty()) return;

    QString username = activeUsers[socket];
    QString room = registeredUsers[username].currentRoom;
    if (room.isEmpty()) {
        sendError(socket, "You must join a room first");
        return;
    }

    broadcastToRoom(room, { {"type", "message"}, {"sender", username}, {"text", text} });
    qDebug() << username << "sent message in" << room << ":" << text;
}

void ChatServer::handleDisconnection(QTcpSocket* socket) {
    if (activeUsers.contains(socket)) {
        QString username = activeUsers[socket];
        QString room = registeredUsers[username].currentRoom;

        if (!room.isEmpty() && chatRooms.contains(room)) {
            chatRooms[room].participants.remove(socket);
            broadcastToRoom(room, { {"type", "message"}, {"text", username + " has left the room"} });
        }

        activeUsers.remove(socket);
        qDebug() << username << "disconnected";
    }

    socket->deleteLater();
}

void ChatServer::broadcastToRoom(const QString& room, const QJsonObject& message) {
    if (!chatRooms.contains(room)) return;

    QByteArray data = QJsonDocument(message).toJson();
    for (QTcpSocket* socket : chatRooms[room].participants) {
        if (socket->state() == QAbstractSocket::ConnectedState) {
            socket->write(data);
        }
    }
}

void ChatServer::sendToClient(QTcpSocket* socket, const QJsonObject& message) {
    if (socket->state() == QAbstractSocket::ConnectedState) {
        socket->write(QJsonDocument(message).toJson());
    }
}

void ChatServer::sendError(QTcpSocket* socket, const QString& message) {
    sendToClient(socket, { {"type", "error"}, {"message", message} });
}

void ChatServer::broadcastRoomList() {
    QJsonObject roomsMsg;
    roomsMsg["type"] = "roomList";
    QJsonArray roomArray;
    for (const auto& room : chatRooms.keys()) {
        roomArray.append(room);
    }
    roomsMsg["rooms"] = roomArray;

    for (QTcpSocket* socket : activeUsers.keys()) {
        sendToClient(socket, roomsMsg);
    }
}

void ChatServer::sendRoomListToClient(QTcpSocket* socket) {
    QJsonObject roomsMsg;
    roomsMsg["type"] = "roomList";
    QJsonArray roomArray;
    for (const auto& room : chatRooms.keys()) {
        roomArray.append(room);
    }
    roomsMsg["rooms"] = roomArray;

    sendToClient(socket, roomsMsg);
}
