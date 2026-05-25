QT += core test network websockets dbus multimedia

CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp

HEADERS += ../src/message.h \
           ../src/messagestore.h \
           ../src/sailpushclient.h \
           ../src/websocketmanager.h \
           ../src/credentialstore.h \
           ../src/networkmonitor.h \
           ../src/cpukeepalive.h \
           ../src/soundplayer.h

INCLUDEPATH += ../src

DEFINES += SRCDIR=\\\"$$PWD/\\\"
