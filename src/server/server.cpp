#include "server.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QDebug>

ChatServer::ChatServer(QObject *parent) : QTcpServer(parent) {
    // Create default public room
    ChatRoom publicRoom;
    publicRoom.name = "Public";
    chatRooms["Public"] = publicRoom;
    
    // Create directory for file transfers if it doesn't exist
    QDir().mkdir("uploads");
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
        
        // Send available rooms list
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
    else if (type == "file") {
        handleFileTransfer(socket, msg);
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
    
    // Add to public room by default
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
    
    // Update room list for all clients
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
    
    // Check password if room is private
    if (!room.password.isEmpty() && data["password"].toString() != room.password) {
        sendError(socket, "Invalid room password");
        return;
    }
    
    // Remove from current room if any
    QString currentRoom = registeredUsers[activeUsers[socket]].currentRoom;
    if (!currentRoom.isEmpty()) {
        chatRooms[currentRoom].participants.remove(socket);
    }
    
    // Add to new room
    room.participants.insert(socket);
    registeredUsers[activeUsers[socket]].currentRoom = roomName;
    
    // Notify room members
    QJsonObject notification;
    notification["type"] = "message";
    notification["text"] = activeUsers[socket] + " has joined the room";
    broadcastToRoom(roomName, notification);
    
    qDebug() << activeUsers[socket] << "joined room:" << roomName;
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
}

void ChatServer::handleFileTransfer(QTcpSocket* socket, const QJsonObject& data) {
    if (!activeUsers.contains(socket)) {
        sendError(socket, "You must be logged in");
        return;
    }
    
    QString fileType = data["fileType"].toString();
    
    if (fileType == "start") {
        fileBuffers[socket].clear();
    }
    else if (fileType == "chunk") {
        QByteArray chunk = QByteArray::fromBase64(data["data"].toString().toLatin1());
        fileBuffers[socket].append(chunk);
    }
    else if (fileType == "end") {
        QString filename = "uploads/" + data["filename"].toString();
        QFile file(filename);
        
        if (file.open(QIODevice::WriteOnly)) {
            file.write(fileBuffers[socket]);
            file.close();
            
            // Notify room members about the file
            QString room = registeredUsers[activeUsers[socket]].currentRoom;
            QJsonObject fileMsg;
            fileMsg["type"] = "fileAvailable";
            fileMsg["filename"] = data["filename"].toString();
            fileMsg["sender"] = activeUsers[socket];
            broadcastToRoom(room, fileMsg);
            
            qDebug() << "File saved:" << filename;
        }
        
        fileBuffers.remove(socket);
    }
}

void ChatServer::handleDisconnection(QTcpSocket* socket) {
    if (activeUsers.contains(socket)) {
        QString username = activeUsers[socket];
        QString room = registeredUsers[username].currentRoom;
        
        // Remove from room
        if (!room.isEmpty() && chatRooms.contains(room)) {
            chatRooms[room].participants.remove(socket);
            
            // Notify room members
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