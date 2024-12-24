#pragma once

#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QSet>
#include <QString>

class ChatRoom {
public:
    QString name;                         // 방 이름
    QString password;                     // 방 비밀번호
    QSet<QTcpSocket*> participants;       // 방 참가자

    ChatRoom() {} // 기본 생성자
    ChatRoom(const QString &roomName, const QString &roomPassword)
        : name(roomName), password(roomPassword) {}
};

class User {
public:
    QString username;  // 사용자 이름
    QString password;  // 사용자 비밀번호
    QString currentRoom;  // 현재 참가 중인 방
};

class ChatServer : public QTcpServer {
    Q_OBJECT

public:
    explicit ChatServer(QObject *parent = nullptr);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    QMap<QString, ChatRoom> chatRooms;      // 방 목록
    QMap<QTcpSocket*, QString> activeUsers; // 활성 사용자
    QMap<QString, User> registeredUsers;   // 등록된 사용자

    // 메시지 처리 함수
    void processMessage(QTcpSocket* socket, const QByteArray& data);
    void handleRegistration(QTcpSocket* socket, const QJsonObject& data);
    void handleLogin(QTcpSocket* socket, const QJsonObject& data);
    void handleCreateRoom(QTcpSocket* socket, const QJsonObject& data);
    void handleJoinRoom(QTcpSocket* socket, const QJsonObject& data);
    void handleChatMessage(QTcpSocket* socket, const QJsonObject& data);
    void handleDisconnection(QTcpSocket* socket);

    // 유틸리티 함수
    void broadcastToRoom(const QString& room, const QJsonObject& message);
    void sendToClient(QTcpSocket* socket, const QJsonObject& message);
    void sendError(QTcpSocket* socket, const QString& message);
    void broadcastRoomList();
};


