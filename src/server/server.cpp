#include "server.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

ChatServer::ChatServer(QObject *parent) : QTcpServer(parent) {
    
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

        
        QJsonObject roomsMsg;
        roomsMsg["type"] = "roomList";
        QJsonArray roomArray;
        for (const auto& room : chatRooms.keys()) {
            roomArray.append(room);
        }
        roomsMsg["rooms"] = roomArray;
        sendToClient(clientSocket, roomsMsg);

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
    }
    else if (type == "login") {
        handleLogin(socket, msg);
    }
    else if (type == "createRoom") {
        handleCreateRoom(socket, msg);
    }
    else if (type == "joinRoom") {
        handleJoinRoom(socket, msg);
    }
    else if (type == "message") {
        handleChatMessage(socket, msg);
    }
    else if (type == "fileUploaded") {
        // 새로 추가된 부분: 파일 업로드 알림 처리
        handleFileUploadNotification(socket, msg);
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

    User newUser;
    newUser.username = username;
    newUser.password = password;
    registeredUsers[username] = newUser;

    QJsonObject response;
    response["type"] = "registrationSuccess";
    sendToClient(socket, response);

    qDebug() << "New user registered:" << username;
}

void ChatServer::handleLogin(QTcpSocket* socket, const QJsonObject& data) {
    QString username = data["username"].toString();
    QString password = data["password"].toString();

    if (!registeredUsers.contains(username) || 
        registeredUsers[username].password != password) {
        sendError(socket, "Invalid username or password");
        return;
    }

    activeUsers[socket] = username;

    QJsonObject response;
    response["type"] = "loginSuccess";
    sendToClient(socket, response);

    chatRooms["Public"].participants.insert(socket);

    qDebug() << username << "logged in";
}

void ChatServer::handleCreateRoom(QTcpSocket* socket, const QJsonObject& data) {
    if (!activeUsers.contains(socket)) {
        sendError(socket, "You must be logged in");
        return;
    }

    QString roomName = data["room"].toString();
    if (roomName.isEmpty()) {
        sendError(socket, "Invalid room name");
        return;
    }

    if (chatRooms.contains(roomName)) {
        sendError(socket, "Room already exists");
        return;
    }

    ChatRoom newRoom;
    newRoom.name = roomName;
    newRoom.password = data["password"].toString();
    chatRooms[roomName] = newRoom;

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

    if (!room.password.isEmpty() && data["password"].toString() != room.password) {
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

    QJsonObject notification;
    notification["type"] = "message";
    notification["text"] = username + " has joined the room";
    broadcastToRoom(roomName, notification);

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

    QJsonObject chatMsg;
    chatMsg["type"] = "message";
    chatMsg["sender"] = username;
    chatMsg["text"] = text;
    broadcastToRoom(room, chatMsg);

    qDebug() << username << "sent message in" << room << ":" << text;
}

void ChatServer::handleFileUploadNotification(QTcpSocket* socket, const QJsonObject& data) {
    if (!activeUsers.contains(socket)) {
        sendError(socket, "You must be logged in");
        return;
    }

    QString username = activeUsers[socket];
    QString filename = data["filename"].toString();
    
    // 현재 방의 모든 사용자에게 파일 업로드 알림 전송
    QString currentRoom = registeredUsers[username].currentRoom;
    if (!currentRoom.isEmpty() && chatRooms.contains(currentRoom)) {
        QJsonObject notification;
        notification["type"] = "fileAvailable";
        notification["filename"] = filename;
        notification["uploader"] = username;
        broadcastToRoom(currentRoom, notification);
        
        qDebug() << username << "uploaded file:" << filename << "in room:" << currentRoom;
    }
}


void ChatServer::handleDisconnection(QTcpSocket* socket) {
    if (activeUsers.contains(socket)) {
        QString username = activeUsers[socket];
        QString room = registeredUsers[username].currentRoom;

        if (!room.isEmpty() && chatRooms.contains(room)) {
            chatRooms[room].participants.remove(socket);

            QJsonObject notification;
            notification["type"] = "message";
            notification["text"] = username + " has left the room";
            broadcastToRoom(room, notification);
        }

        activeUsers.remove(socket);
        qDebug() << username << "disconnected";
    }

    socket->deleteLater();
}

void ChatServer::broadcastToRoom(const QString& room, const QJsonObject& message) {
    if (!chatRooms.contains(room)) return;

    QJsonDocument doc(message);
    QByteArray data = doc.toJson();

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
    QJsonObject errorMsg;
    errorMsg["type"] = "error";
    errorMsg["message"] = message;
    sendToClient(socket, errorMsg);
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
