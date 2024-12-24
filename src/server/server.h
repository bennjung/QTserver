#pragma once
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QString>
#include <QSet>
#include <QJsonObject>

// Structure to store chat room information
struct ChatRoom {
    QString name;                      // Chat room name
    QSet<QTcpSocket*> participants;    // List of participants
    QString password;                  // Password (optional)
};

// Structure to store user information
struct User {
    QString username;     // Username
    QString password;     // Password
    QString currentRoom;  // Current room the user is in
};

class ChatServer : public QTcpServer {
    Q_OBJECT
public:
    explicit ChatServer(QObject *parent = nullptr);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void processMessage(QTcpSocket* socket, const QByteArray& data);
    void handleDisconnection(QTcpSocket* socket);

private:
    // Data storage
    QMap<QString, User> registeredUsers;           // List of registered users
    QMap<QString, ChatRoom> chatRooms;             // List of chat rooms
    QMap<QTcpSocket*, QString> activeUsers;        // Currently connected users

    // Message handlers
    void handleRegistration(QTcpSocket* socket, const QJsonObject& data);
    void handleLogin(QTcpSocket* socket, const QJsonObject& data);
    void handleCreateRoom(QTcpSocket* socket, const QJsonObject& data);
    void handleJoinRoom(QTcpSocket* socket, const QJsonObject& data);
    void handleChatMessage(QTcpSocket* socket, const QJsonObject& data);

    // Utility functions
    void broadcastToRoom(const QString& room, const QJsonObject& message);
    void sendToClient(QTcpSocket* socket, const QJsonObject& message);
    void sendError(QTcpSocket* socket, const QString& message);
    void broadcastRoomList();
    void sendRoomListToClient(QTcpSocket* socket);
};
