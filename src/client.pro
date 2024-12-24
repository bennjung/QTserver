QT += core network widgets
CONFIG += c++11

TARGET = chat_client
CONFIG -= app_bundle

# 빌드 디렉토리 설정
DESTDIR = $$PWD/build/client
OBJECTS_DIR = $$PWD/build/client/.obj
MOC_DIR = $$PWD/build/client/.moc

SOURCES += \
    client/main.cpp \
    client/client.cpp

HEADERS += \
    client/client.h