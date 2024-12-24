#include "client.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QNetworkRequest>
#include <QNetworkReply>


ChatClient::ChatClient(QWidget *parent) : QMainWindow(parent) {
    setupUI();
    connectToServer();
    setupFtpClient();
}

void ChatClient::setupUI() {
    setWindowTitle("Chat Client");
    setGeometry(100, 100, 800, 600);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Login/Register area
    QHBoxLayout *authLayout = new QHBoxLayout();
    usernameInput = new QLineEdit(this);
    passwordInput = new QLineEdit(this);
    passwordInput->setEchoMode(QLineEdit::Password);
    
    QPushButton *loginButton = new QPushButton("Login", this);
    QPushButton *registerButton = new QPushButton("Register", this);
    
    authLayout->addWidget(new QLabel("Username:"));
    authLayout->addWidget(usernameInput);
    authLayout->addWidget(new QLabel("Password:"));
    authLayout->addWidget(passwordInput);
    authLayout->addWidget(loginButton);
    authLayout->addWidget(registerButton);
    
    // Room management
    QHBoxLayout *roomLayout = new QHBoxLayout();
    roomList = new QComboBox(this);
    QPushButton *createRoomButton = new QPushButton("Create Room", this);
    QPushButton *joinRoomButton = new QPushButton("Join Room", this);
    
    roomLayout->addWidget(roomList);
    roomLayout->addWidget(createRoomButton);
    roomLayout->addWidget(joinRoomButton);

    // Chat area
    chatArea = new QTextEdit(this);
    chatArea->setReadOnly(true);

    // Message input
    QHBoxLayout *messageLayout = new QHBoxLayout();
    messageInput = new QLineEdit(this);
    QPushButton *sendButton = new QPushButton("Send", this);
    
    messageLayout->addWidget(messageInput);
    messageLayout->addWidget(sendButton);

    // FTP
    QHBoxLayout *fileLayout = new QHBoxLayout();
    fileList = new QListWidget(this);
    
    QPushButton *uploadButton = new QPushButton("Upload File", this);
    QPushButton *downloadButton = new QPushButton("Download File", this);
    
    fileLayout->addWidget(fileList);
    QVBoxLayout *fileButtonLayout = new QVBoxLayout();
    fileButtonLayout->addWidget(uploadButton);
    fileButtonLayout->addWidget(downloadButton);
    fileLayout->addLayout(fileButtonLayout);

    // Add all layouts to main layout
    mainLayout->addLayout(authLayout);
    mainLayout->addLayout(roomLayout);
    mainLayout->addWidget(chatArea);
    mainLayout->addLayout(messageLayout);
    mainLayout->addLayout(fileLayout);

    setCentralWidget(centralWidget);

    // Connect signals/slots
    connect(loginButton, &QPushButton::clicked, this, &ChatClient::handleLogin);
    connect(registerButton, &QPushButton::clicked, this, &ChatClient::handleRegistration);
    connect(createRoomButton, &QPushButton::clicked, this, &ChatClient::handleCreateRoom);
    connect(joinRoomButton, &QPushButton::clicked, this, &ChatClient::handleJoinRoom);
    connect(sendButton, &QPushButton::clicked, this, &ChatClient::sendMessage);
    connect(uploadButton, &QPushButton::clicked, this, &ChatClient::uploadFile);
    connect(downloadButton, &QPushButton::clicked, this, &ChatClient::downloadFile);
    connect(messageInput, &QLineEdit::returnPressed, this, &ChatClient::sendMessage);
}



void ChatClient::connectToServer() {
    socket = new QTcpSocket(this);

    connect(socket, &QTcpSocket::connected, [this]() {
        chatArea->append("Connected to chat server");
    });

    connect(socket, &QTcpSocket::disconnected, [this]() {
        chatArea->append("Disconnected from chat server");
    });

    connect(socket, &QTcpSocket::readyRead, [this]() {
        QByteArray data = socket->readAll();
        processServerMessage(data);
    });

    socket->connectToHost("127.0.0.1", 12345);
    if (!socket->waitForConnected(3000)) {
        QMessageBox::critical(this, "Error", "Could not connect to chat server");
    }
}

void ChatClient::setupFtpClient() {
    networkManager = new QNetworkAccessManager(this);

    connect(networkManager, &QNetworkAccessManager::finished, this, &ChatClient::onFtpReplyFinished);
}

void ChatClient::uploadFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Select File to Upload");
    if (fileName.isEmpty()) return;

    QFile *file = new QFile(fileName);
    if (!file->open(QIODevice::ReadOnly)) {
        chatArea->append("Failed to open file: " + fileName);
        delete file;
        return;
    }

    // FTP URL 생성
    QUrl url("ftp://127.0.0.1/" + QFileInfo(fileName).fileName());
    url.setUserName("ftpuser");  // FTP 사용자 이름
    url.setPassword("zozo99");  // FTP 비밀번호

    // QNetworkRequest 생성
    QNetworkRequest request(url);

    // 업로드 시작
    QNetworkReply *reply = networkManager->put(request, file);

    // 업로드 진행 상태 추적
    connect(reply, &QNetworkReply::uploadProgress, this, &ChatClient::updateDataTransferProgress);

    // 업로드 완료 후 처리
    connect(reply, &QNetworkReply::finished, this, [reply, file, this]() {
        reply->deleteLater();
        file->close();
        file->deleteLater();
        chatArea->append("Upload complete!");
    });

    chatArea->append("Uploading: " + fileName);;
}

void ChatClient::downloadFile() {
    if (!fileList->currentItem()) {
        QMessageBox::warning(this, "Error", "Please select a file to download");
        return;
    }

    QString fileName = fileList->currentItem()->text();
    QString saveFileName = QFileDialog::getSaveFileName(this, "Save File As", fileName);

    if (saveFileName.isEmpty()) return;

    // FTP URL 생성
    QUrl url("ftp://127.0.0.1/" + fileName);
    url.setUserName("ftpuser");  // FTP 사용자 이름
    url.setPassword("zozo99");  // FTP 비밀번호

    // QNetworkRequest 생성
    QNetworkRequest request(url);

    // 다운로드 시작
    QNetworkReply *reply = networkManager->get(request);

    // 다운로드 진행 상태 추적
    QFile *file = new QFile(saveFileName);
    if (!file->open(QIODevice::WriteOnly)) {
        chatArea->append("Failed to create file: " + saveFileName);
        delete file;
        return;
    }

    connect(reply, &QNetworkReply::readyRead, [reply, file]() {
        file->write(reply->readAll());
    });

    connect(reply, &QNetworkReply::finished, this, [reply, file, this]() {
        file->close();
        file->deleteLater();
        reply->deleteLater();
        chatArea->append("Download complete!");
    });

    chatArea->append("Downloading: " + fileName);
}

void ChatClient::onFtpReplyFinished(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        chatArea->append("FTP Error: " + reply->errorString());
    }

    reply->deleteLater();

    if (progressDialog) {
        progressDialog->hide();
        delete progressDialog;
        progressDialog = nullptr;
    }
}

void ChatClient::updateDataTransferProgress(qint64 done, qint64 total) {
    if (progressDialog) {
        progressDialog->setValue(done * 100 / total);
    }
}

void ChatClient::handleLogin() {
    QJsonObject loginMsg;
    loginMsg["type"] = "login";
    loginMsg["username"] = usernameInput->text();
    loginMsg["password"] = passwordInput->text();
    
    sendJsonMessage(loginMsg);
}

void ChatClient::handleRegistration() {
    QJsonObject regMsg;
    regMsg["type"] = "register";
    regMsg["username"] = usernameInput->text();
    regMsg["password"] = passwordInput->text();
    
    sendJsonMessage(regMsg);
}

void ChatClient::handleCreateRoom() {
    QString roomName = QInputDialog::getText(this, "Create Room", "Room name:");
    if (!roomName.isEmpty()) {
        QJsonObject createMsg;
        createMsg["type"] = "createRoom";
        createMsg["room"] = roomName;
        
        // Optional: Add password for private rooms
        bool ok;
        QString password = QInputDialog::getText(this, "Room Password", 
            "Password (leave empty for public room):", 
            QLineEdit::Password, "", &ok);
        if (ok && !password.isEmpty()) {
            createMsg["password"] = password;
        }
        
        sendJsonMessage(createMsg);
    }
}

void ChatClient::handleJoinRoom() {
    if (roomList->currentText().isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select a room");
        return;
    }
    
    QJsonObject joinMsg;
    joinMsg["type"] = "joinRoom";
    joinMsg["room"] = roomList->currentText();
    
    sendJsonMessage(joinMsg);
}

void ChatClient::sendMessage() {
    QString text = messageInput->text();
    if (text.isEmpty()) return;
    
    QJsonObject chatMsg;
    chatMsg["type"] = "message";
    chatMsg["text"] = text;
    
    sendJsonMessage(chatMsg);
    messageInput->clear();
}

void ChatClient::sendFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Select File");
    if (fileName.isEmpty()) return;
    
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Cannot open file");
        return;
    }
    
    // Send file start message
    QJsonObject startMsg;
    startMsg["type"] = "file";
    startMsg["fileType"] = "start";
    startMsg["filename"] = QFileInfo(fileName).fileName();
    sendJsonMessage(startMsg);
    
    // Send file in chunks
    const int chunkSize = 1024 * 1024; // 1MB chunks
    while (!file.atEnd()) {
        QByteArray chunk = file.read(chunkSize);
        QJsonObject chunkMsg;
        chunkMsg["type"] = "file";
        chunkMsg["fileType"] = "chunk";
        chunkMsg["data"] = QString(chunk.toBase64());
        sendJsonMessage(chunkMsg);
    }
    
    // Send end message
    QJsonObject endMsg;
    endMsg["type"] = "file";
    endMsg["fileType"] = "end";
    endMsg["filename"] = QFileInfo(fileName).fileName();
    sendJsonMessage(endMsg);
    
    file.close();
}



void ChatClient::processServerMessage(const QByteArray& data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return;
    
    QJsonObject msg = doc.object();
    QString type = msg["type"].toString();
    
    if (type == "message") {
        QString sender = msg["sender"].toString();
        QString text = msg["text"].toString();
        chatArea->append(QString("%1: %2").arg(sender, text));
    }
    else if (type == "fileAvailable") {
        QString filename = msg["filename"].toString();
        fileList->addItem(filename);
    }
    else if (type == "error") {
        QMessageBox::warning(this, "Error", msg["message"].toString());
    }
    else if (type == "roomList") {
        roomList->clear();
        for (const auto& room : msg["rooms"].toArray()) {
            roomList->addItem(room.toString());
        }
    }
}

void ChatClient::sendJsonMessage(const QJsonObject& message) {
    QJsonDocument doc(message);
    socket->write(doc.toJson());
}
