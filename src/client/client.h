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
#include <QNetworkAccessManager>
#include <QProgressDialog>
#include <QTimer>
#include <QSettings>
#include <QInputDialog>
#include <QString>
#include <QMessageBox>
#include <QFileInfo>
#include <QFile>

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
    

    // 파일 전송 관련 슬롯
    void uploadFile();
    void downloadFile();
    void updateDataTransferProgress(qint64 done, qint64 total);
    void onFtpReplyFinished(QNetworkReply *reply);
    void updateFileList();

private:
    // UI 컴포넌트
    QLineEdit *usernameInput;
    QLineEdit *passwordInput;
    QComboBox *roomList;
    QTextEdit *chatArea;
    QLineEdit *messageInput;
    QListWidget *fileList;

    // 네트워크 컴포넌트
    QTcpSocket *socket;
    QNetworkAccessManager *networkManager;
    QProgressDialog *progressDialog;
    QTimer *fileListTimer;

    //FTP 설정
    QString ftpHost;
    QString ftpUsername;
    QString ftpPassword;
    // 초기화 함수
    void setupUI();
    void setupFtpClient();  
    void connectToServer();
    void sendJsonMessage(const QJsonObject& message);
};

