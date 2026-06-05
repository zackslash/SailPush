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
    dbus-1/com.zackslash.sailpush.service

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

# Translations — European languages
TRANSLATIONS += \
    translations/sailpush_de.ts \
    translations/sailpush_fr.ts \
    translations/sailpush_es.ts \
    translations/sailpush_it.ts \
    translations/sailpush_pt.ts \
    translations/sailpush_nl.ts \
    translations/sailpush_sv.ts \
    translations/sailpush_nb.ts \
    translations/sailpush_da.ts \
    translations/sailpush_fi.ts \
    translations/sailpush_is.ts \
    translations/sailpush_pl.ts \
    translations/sailpush_cs.ts \
    translations/sailpush_sk.ts \
    translations/sailpush_hu.ts \
    translations/sailpush_ro.ts \
    translations/sailpush_hr.ts \
    translations/sailpush_sr.ts \
    translations/sailpush_sl.ts \
    translations/sailpush_bg.ts \
    translations/sailpush_el.ts \
    translations/sailpush_tr.ts \
    translations/sailpush_et.ts \
    translations/sailpush_lv.ts \
    translations/sailpush_lt.ts \
    translations/sailpush_sq.ts \
    translations/sailpush_mk.ts \
    translations/sailpush_bs.ts \
    translations/sailpush_mt.ts \
    translations/sailpush_ga.ts \
    translations/sailpush_cy.ts \
    translations/sailpush_eu.ts \
    translations/sailpush_ca.ts \
    translations/sailpush_gl.ts \
    translations/sailpush_uk.ts \
    translations/sailpush_ru.ts \
    translations/sailpush_be.ts \
    translations/sailpush_hy.ts \
    translations/sailpush_ka.ts

systemd_service.path = /usr/lib/systemd/user
systemd_service.files = systemd/sailpush.service
INSTALLS += systemd_service

notification_category.path = /usr/share/lipstick/notificationcategories
notification_category.files = notifications/x-sailpush.conf
INSTALLS += notification_category

dbus_service.path = /usr/share/dbus-1/services
dbus_service.files = dbus-1/com.zackslash.sailpush.service
INSTALLS += dbus_service
