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
#include <QJsonArray>
#include <QFtp>
#include <QProgressDialog>
#include <QUrl>

class ChatClient : public QMainWindow {
    Q_OBJECT
public:
    ChatClient(QWidget *parent = nullptr);

private slots:
    // 채팅 관련 슬롯
    void handleLogin();
    void handleRegistration();
    void handleCreateRoom();
    void handleJoinRoom();
    void sendMessage();
    void processServerMessage(const QByteArray& data);

    // FTP 관련 슬롯
    void uploadFile();
    void downloadFile();
    void ftpCommandFinished(int id, bool error);
    void updateDataTransferProgress(qint64 done, qint64 total);

private:
    // UI components
    QLineEdit *usernameInput;
    QLineEdit *passwordInput;
    QComboBox *roomList;
    QTextEdit *chatArea;
    QLineEdit *messageInput;
    QListWidget *fileList;
    
    // 채팅 서버 연결
    QTcpSocket *socket;
    void connectToServer();
    void sendJsonMessage(const QJsonObject& message);

    // FTP 관련 멤버
    QFtp *ftp;
    QProgressDialog *progressDialog;
    bool ftpLoggedIn;
    void connectToFtpServer();
    
    // UI 설정
    void setupUI();
};