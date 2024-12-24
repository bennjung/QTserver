#pragma once
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QString>
#include <QSet>

// 채팅방 정보를 저장하는 구조체
struct ChatRoom {
    QString name;                      // 채팅방 이름
    QSet<QTcpSocket*> participants;    // 참가자 목록
    QString password;                  // 비밀번호 (선택적)
};

// 사용자 정보를 저장하는 구조체
struct User {
    QString username;     // 사용자 이름
    QString password;     // 비밀번호
    QString currentRoom;  // 현재 참여중인 방
};

class ChatServer : public QTcpServer {
    Q_OBJECT
public:
    ChatServer(QObject *parent = nullptr);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void processMessage(QTcpSocket* socket, const QByteArray& data);
    void handleDisconnection(QTcpSocket* socket);

private:
    // 데이터 저장소
    QMap<QString, User> registeredUsers;           // 등록된 사용자 목록
    QMap<QString, ChatRoom> chatRooms;            // 채팅방 목록
    QMap<QTcpSocket*, QString> activeUsers;        // 현재 접속중인 사용자
    QMap<QTcpSocket*, QByteArray> fileBuffers;     // 파일 전송용 버퍼

    // 메시지 핸들러
    void handleRegistration(QTcpSocket* socket, const QJsonObject& data);
    void handleLogin(QTcpSocket* socket, const QJsonObject& data);
    void handleCreateRoom(QTcpSocket* socket, const QJsonObject& data);
    void handleJoinRoom(QTcpSocket* socket, const QJsonObject& data);
    void handleChatMessage(QTcpSocket* socket, const QJsonObject& data);
    void handleFileTransfer(QTcpSocket* socket, const QJsonObject& data);

    // 유틸리티 함수
    void broadcastToRoom(const QString& room, const QJsonObject& message);
    void sendToClient(QTcpSocket* socket, const QJsonObject& message);
    void sendError(QTcpSocket* socket, const QString& message);
    void broadcastRoomList();
};