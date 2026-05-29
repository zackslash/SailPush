TARGET = sailpush

CONFIG += sailfishapp
QT += core gui quick qml network websockets dbus multimedia

# Auto-detect version from git. CI builds (tarball, no .git) use sed-replaced
# fallback in source; local dev builds get "v0.9.0-42-gabc1234" style strings.
GIT_VERSION = $$system(git describe --tags --always 2>/dev/null)
isEmpty(GIT_VERSION): GIT_VERSION = "dev"
DEFINES += GIT_VERSION=\\\"$$GIT_VERSION\\\"

SOURCES += src/main.cpp \
           src/daemon.cpp \
           src/sailpushclient.cpp \
           src/websocketmanager.cpp \
           src/messagestore.cpp \
           src/credentialstore.cpp \
           src/dbusinterface.cpp \
           src/notificationmanager.cpp \
           src/networkmonitor.cpp \
           src/cpukeepalive.cpp \
           src/loginhelper.cpp \
           src/soundplayer.cpp

HEADERS += src/message.h \
           src/sailpushclient.h \
           src/websocketmanager.h \
           src/messagestore.h \
           src/credentialstore.h \
           src/dbusinterface.h \
           src/notificationmanager.h \
           src/networkmonitor.h \
           src/cpukeepalive.h \
           src/loginhelper.h \
           src/soundplayer.h \
           src/daemon.h

DISTFILES += qml/sailpush.qml \
    qml/cover/CoverPage.qml \
    qml/pages/*.qml \
    qml/components/*.qml \
    qml/js/*.js \
    rpm/sailpush.changes.in \
    rpm/sailpush.changes.run.in \
    rpm/sailpush.spec \
    sailpush.desktop \
    systemd/sailpush.service \
    notifications/x-sailpush.conf \
    dbus-1/net.sailpush.Sailfish.service

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

systemd_service.path = /usr/lib/systemd/user
systemd_service.files = systemd/sailpush.service
INSTALLS += systemd_service

notification_category.path = /usr/share/lipstick/notificationcategories
notification_category.files = notifications/x-sailpush.conf
INSTALLS += notification_category

dbus_service.path = /usr/share/dbus-1/services
dbus_service.files = dbus-1/net.sailpush.Sailfish.service
INSTALLS += dbus_service
