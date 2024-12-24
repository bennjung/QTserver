QT += core network widgets
CONFIG += c++11

TARGET = chat_client
CONFIG -= app_bundle

SOURCES += \
    client/main.cpp \
    client/client.cpp

HEADERS += \
    client/client.h