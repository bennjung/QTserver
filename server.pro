QT += core network
QT -= gui

TARGET = chat_server
CONFIG += c++11 console
CONFIG -= app_bundle

SOURCES += \
    server/main.cpp \
    server/server.cpp

HEADERS += \
    server/server.h