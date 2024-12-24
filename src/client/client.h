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
#include <QNetworkReply>
#include <QProgressDialog>
#include <QUrl>

class ChatClient : public QMainWindow {
    Q_OBJECT
public:
    ChatClient(QWidget *parent = nullptr);

private slots:
    // Chat-related slots
    void handleLogin();
    void handleRegistration();
    void handleCreateRoom();
    void handleJoinRoom();
    void sendMessage();
    void processServerMessage(const QByteArray& data);

    // File transfer-related slots
    void uploadFile();
    void downloadFile();
    void handleUploadFinished();
    void handleDownloadFinished();
    void updateDataTransferProgress(qint64 bytesReceived, qint64 bytesTotal);

private:
    // UI components
    QLineEdit *usernameInput;
    QLineEdit *passwordInput;
    QComboBox *roomList;
    QTextEdit *chatArea;
    QLineEdit *messageInput;
    QListWidget *fileList;

    // Chat server connection
    QTcpSocket *socket;
    void connectToServer();
    void sendJsonMessage(const QJsonObject& message);

    // File transfer components
    QNetworkAccessManager *networkManager;
    QProgressDialog *progressDialog;
    void setupNetworkManager();

    // UI setup
    void setupUI();
};
