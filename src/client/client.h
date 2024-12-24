#pragma once
#include <QMainWindow>
#include <QTcpSocket>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QInputDialog>    
#include <QMessageBox> 

class ChatClient : public QMainWindow {
    Q_OBJECT
public:
    ChatClient(QWidget *parent = nullptr);

private slots:
    void handleLogin();
    void handleRegistration();
    void handleCreateRoom();
    void handleJoinRoom();
    void sendFile();
    void downloadFile();
    void processServerMessage(const QByteArray& data);
    void sendMessage();

private:
    // UI components
    QLineEdit *usernameInput;
    QLineEdit *passwordInput;
    QComboBox *roomList;
    QTextEdit *chatArea;
    QLineEdit *messageInput;
    QListWidget *fileList;
    
    // Network
    QTcpSocket *socket;
    QByteArray fileBuffer;
    
    void setupUI();
    void connectToServer();
    void sendJsonMessage(const QJsonObject& message);
};