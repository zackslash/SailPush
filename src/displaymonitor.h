#ifndef DISPLAYMONITOR_H
#define DISPLAYMONITOR_H

#include <QObject>
#include <QDBusConnection>

// Monitors MCE display status (screen on/off) so the daemon can drop the
// WebSocket when the screen is off and rely on polling, then reconnect for
// instant delivery when the screen comes back on. Mirrors the CpuKeepalive /
// NetworkMonitor pattern: system bus, com.nokia.mce service.
class DisplayMonitor : public QObject {
    Q_OBJECT

public:
    explicit DisplayMonitor(QObject *parent = nullptr);
    ~DisplayMonitor();

    bool isDisplayOn() const;

signals:
    void displayOn();
    void displayOff();

private slots:
    void onDisplayStatusInd(const QString &status);

private:
    void queryInitialState();
    void applyStatus(const QString &status);

    QDBusConnection m_conn;
    bool m_displayOn;
};

#endif // DISPLAYMONITOR_H
