#include <sailfishapp.h>
#include <QQuickView>
#include <QGuiApplication>
#include <QCoreApplication>
#include <QQmlContext>
#include <QLoggingCategory>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusReply>
#include <QTimer>
#include <QProcess>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QTranslator>
#include <QLocale>
#include "loginhelper.h"
#include "daemon.h"
#include "sailpushclient.h"

Q_LOGGING_CATEGORY(lcMain, "net.sailpush.sailfish.main")

static const QString DBUS_SERVICE = "net.sailpush.Sailfish";

static void loadTranslations(QCoreApplication *app)
{
    QLocale locale = QLocale::system();
    auto *translator = new QTranslator(app);
    QString transDir = SailfishApp::pathTo("translations").toLocalFile();
    if (translator->load(locale.name(), "sailpush", "_", transDir)) {
        app->installTranslator(translator);
        qCInfo(lcMain) << "Loaded translations for" << locale.name();
    } else {
        // Try language-only (e.g. "de" from "de_DE")
        QString lang = locale.name().left(locale.name().indexOf('_'));
        if (!lang.isEmpty() && translator->load(lang, "sailpush", "_", transDir)) {
            app->installTranslator(translator);
            qCInfo(lcMain) << "Loaded translations for" << lang;
        } else {
            qCInfo(lcMain) << "No translations found for" << locale.name() << "- using English";
            delete translator;
        }
    }
}

int runDaemon(int argc, char *argv[]);

static bool isDaemonRegistered(QDBusConnectionInterface *iface)
{
    if (!iface) return false;
    QDBusReply<bool> reg = iface->isServiceRegistered(DBUS_SERVICE);
    return reg.isValid() && reg.value();
}

static void startDaemonDirect(QDBusConnectionInterface *iface)
{
    // First, retry D-Bus activation (works when .service file is installed via RPM)
    if (iface) {
        qCInfo(lcMain) << "Retrying D-Bus activation for daemon...";
        iface->startService(DBUS_SERVICE);
    }

    // After 2s, if daemon still isn't running, launch directly via QProcess.
    // Inside Sailjail, the daemon won't have system D-Bus access (MCE, ConnMan)
    // but core functionality (WebSocket, HTTP API) still works.
    QTimer::singleShot(2000, [iface]() {
        if (!isDaemonRegistered(iface)) {
            QString daemonPath = QCoreApplication::arguments().value(0);
            if (daemonPath.isEmpty() || daemonPath.contains("invoker") || !QFile::exists(daemonPath))
                daemonPath = "/usr/bin/sailpush";

            qCWarning(lcMain) << "D-Bus activation failed, launching daemon directly:" << daemonPath;
            QProcess::startDetached(daemonPath, QStringList() << "--daemon");
        }
    });
}

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--daemon") == 0 || strcmp(argv[i], "--background") == 0) {
            return runDaemon(argc, argv);
        }
    }

    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    loadTranslations(app.data());
    QScopedPointer<QQuickView> view(SailfishApp::createView());

    LoginHelper *loginHelper = new LoginHelper(view.data());
    view->rootContext()->setContextProperty("loginHelper", loginHelper);

    QDBusConnection sessionBus = QDBusConnection::sessionBus();
    QDBusConnectionInterface *sessionIface = sessionBus.interface();

    if (!sessionIface) {
        qCWarning(lcMain) << "No session bus available";
    }

    // Ensure the daemon is running. We do NOT block the main thread —
    // the QML DaemonConnector handles async discovery via NameOwnerChanged
    // signals and a polling timer.
    //
    // Strategy: try D-Bus activation first, then fall back to direct
    // process launch after a short delay if the daemon doesn't appear.
    if (!isDaemonRegistered(sessionIface)) {
        qCInfo(lcMain) << "No daemon running, trying D-Bus activation...";
        if (sessionIface) {
            sessionIface->startService(DBUS_SERVICE);
        }

        // After 2s, if daemon still isn't registered, try harder
        QTimer::singleShot(2000, [sessionIface]() {
            if (!isDaemonRegistered(sessionIface)) {
                qCWarning(lcMain) << "D-Bus activation did not start daemon, retrying";
                startDaemonDirect(sessionIface);
            }
        });
    } else {
        qCInfo(lcMain) << "Daemon already running";
    }

    view->rootContext()->setContextProperty("daemonMessages", QVariantList());
    view->rootContext()->setContextProperty("unreadCount", 0);
    view->rootContext()->setContextProperty("connectionState", "disconnected");

#ifdef GIT_VERSION
    view->rootContext()->setContextProperty("appVersion", QStringLiteral(GIT_VERSION));
#else
    view->rootContext()->setContextProperty("appVersion", QStringLiteral("dev"));
#endif

    view->rootContext()->setContextProperty("appName", QString::fromLatin1(SailPushClient::APP_DISPLAY_NAME));

    view->setSource(SailfishApp::pathTo("qml/sailpush.qml"));
    view->show();

    return app->exec();
}

int runDaemon(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    loadTranslations(&app);
    QLoggingCategory::setFilterRules("net.sailpush.sailfish.*=true");

    // Log to file AND stderr for debugging
    // (daemon runs as systemd service — logs go to journald; file accessible via file manager)
    QString logPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                      + "/net.sailpush/sailfish/daemon.log";
    QDir().mkpath(QFileInfo(logPath).absolutePath());
    static FILE *logFile = fopen(logPath.toLocal8Bit().constData(), "a");
    if (logFile) {
        qInstallMessageHandler([](QtMsgType type, const QMessageLogContext &ctx, const QString &msg) {
            QByteArray localMsg = msg.toLocal8Bit();
            const char *level = "I";
            if (type == QtWarningMsg) level = "W";
            else if (type == QtCriticalMsg || type == QtFatalMsg) level = "E";
            else if (type == QtDebugMsg) level = "D";
            QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
            // Write to file
            fprintf(logFile, "[%s] %s %s\n", timestamp.toLocal8Bit().constData(), level, localMsg.constData());
            fflush(logFile);
            // Also write to stderr (so Qt Creator Application Output and journald still work)
            fprintf(stderr, "[%s] %s %s\n", timestamp.toLocal8Bit().constData(), level, localMsg.constData());
        });
    }

    Daemon daemon;
    daemon.start();
    return app.exec();
}
