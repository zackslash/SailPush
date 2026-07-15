#include "displaymonitor.h"
#include <QDBusInterface>
#include <QDBusReply>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcDisplayMonitor, "com.zackslash.sailpush.display")

// MCE D-Bus constants (verified against sailfishos/mce-dev dbus-names.h +
// mode-names.h): signal "display_status_ind" emits one string arg whose value
// is "on", "dimmed", or "off". Service com.nokia.mce is on the system bus.
static const char *MCE_SERVICE = "com.nokia.mce";
static const char *MCE_SIGNAL_PATH = "/com/nokia/mce/signal";
static const char *MCE_SIGNAL_IF = "com.nokia.mce.signal";
static const char *MCE_DISPLAY_SIG = "display_status_ind";
static const char *MCE_REQUEST_PATH = "/com/nokia/mce/request";
static const char *MCE_REQUEST_IF = "com.nokia.mce.request";
static const char *MCE_DISPLAY_GET = "get_display_status";

DisplayMonitor::DisplayMonitor(QObject *parent)
    : QObject(parent)
    , m_conn(QDBusConnection::systemBus())
    , m_displayOn(true)   // safe default: assume on if MCE unavailable (no regression)
{
    if (!m_conn.connect(MCE_SERVICE, MCE_SIGNAL_PATH, MCE_SIGNAL_IF,
                        MCE_DISPLAY_SIG, this,
                        SLOT(onDisplayStatusInd(QString)))) {
        qCWarning(lcDisplayMonitor) << "Failed to subscribe to" << MCE_DISPLAY_SIG;
    }

    // Defer the initial query via a queued invocation so any displayOff signal
    // is emitted AFTER the daemon has wired its signal connections (which
    // happen later in the Daemon constructor body). Without this, a startup
    // with the screen already off would emit to zero receivers.
    QMetaObject::invokeMethod(this, &DisplayMonitor::queryInitialState, Qt::QueuedConnection);
}

DisplayMonitor::~DisplayMonitor()
{
    m_conn.disconnect(MCE_SERVICE, MCE_SIGNAL_PATH, MCE_SIGNAL_IF,
                      MCE_DISPLAY_SIG, this,
                      SLOT(onDisplayStatusInd(QString)));
}

bool DisplayMonitor::isDisplayOn() const
{
    return m_displayOn;
}

void DisplayMonitor::onDisplayStatusInd(const QString &status)
{
    applyStatus(status);
}

void DisplayMonitor::queryInitialState()
{
    QDBusInterface mce(MCE_SERVICE, MCE_REQUEST_PATH, MCE_REQUEST_IF, m_conn);
    if (!mce.isValid()) {
        qCWarning(lcDisplayMonitor) << "MCE request interface not available, assuming display on";
        return;
    }

    QDBusReply<QString> reply = mce.call(MCE_DISPLAY_GET);
    if (!reply.isValid()) {
        qCWarning(lcDisplayMonitor) << "get_display_status failed:" << reply.error().message();
        return;
    }

    applyStatus(reply.value());
}

void DisplayMonitor::applyStatus(const QString &status)
{
    // "on" and "dimmed" both mean the screen is active enough to keep the WS
    // alive; only "off" triggers the sleep-mode polling path.
    bool nowOn = (status != QLatin1String("off"));
    if (nowOn == m_displayOn) {
        return;
    }

    qCInfo(lcDisplayMonitor) << "Display status:" << status << "-> on=" << nowOn;
    m_displayOn = nowOn;
    if (nowOn) {
        emit displayOn();
    } else {
        emit displayOff();
    }
}
