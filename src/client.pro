QT += core network
QT -= gui

TARGET = chat_server
CONFIG += c++11 console
CONFIG -= app_bundle

# 빌드 디렉토리 설정
DESTDIR = $$PWD/build/server
OBJECTS_DIR = $$PWD/build/server/.obj
MOC_DIR = $$PWD/build/server/.moc

SOURCES += \
    server/main.cpp \
    server/server.cpp

HEADERS += \
    server/server.h