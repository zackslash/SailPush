import QtQuick 2.0
import Nemo.DBus 2.0

Item {
    id: root

    property var messages: []
    property int unreadCount: 0
    property string connectionState: "disconnected"
    property bool isRunning: false
    property bool autoStartEnabled: false
    property string daemonError: ""
    property int daemonRetryCount: 0
    property string diagnosticInfo: ""
    signal connectionStateUpdated(string state)
    signal credentialsInvalidated(string reason)

    DBusInterface {
        id: daemonInterface
        service: "net.sailpush.Sailfish"
        path: "/net/sailpush/Sailfish"
        iface: "net.sailpush.Sailfish"
        signalsEnabled: true
    }

    DBusInterface {
        id: dbusDaemon
        service: "org.freedesktop.DBus"
        path: "/org/freedesktop/DBus"
        iface: "org.freedesktop.DBus"
        signalsEnabled: true
    }

    function _refreshMessages() {
        daemonInterface.typedCall("GetMessages", [],
            function(result) { root.messages = result },
            function(error, message) { /* daemon may have just stopped */ }
        )
    }

    function _refreshUnreadCount() {
        daemonInterface.typedCall("GetUnreadCount", [],
            function(result) { root.unreadCount = result },
            function(error, message) { /* daemon may have just stopped */ }
        )
    }

    function _refreshConnectionState() {
        daemonInterface.typedCall("GetConnectionState", [],
            function(result) { root.connectionState = result },
            function(error, message) { /* daemon may have just stopped */ }
        )
    }

    function _refreshAutoStart() {
        daemonInterface.typedCall("GetAutoStartEnabled", [],
            function(result) { root.autoStartEnabled = result },
            function(error, message) { /* daemon may have just stopped */ }
        )
    }

    function _refreshDiagnostics() {
        daemonInterface.typedCall("GetDiagnostics", [],
            function(result) { root.diagnosticInfo = result },
            function(error, message) { root.diagnosticInfo = "Error: " + message }
        )
    }

    function toggleAutoStart() {
        var newValue = !root.autoStartEnabled
        daemonInterface.typedCall("SetAutoStartEnabled", [{ "type": "b", "value": newValue }],
            function() { _refreshAutoStart() },
            function(error, message) { _refreshAutoStart() }
        )
    }

    function _onDaemonFound() {
        if (!root.isRunning) {
            root.isRunning = true
            root.daemonError = ""
            root.daemonRetryCount = 0
            daemonCheckTimer.interval = 10000
            if (!daemonCheckTimer.running) daemonCheckTimer.start()
            daemonInterface.call("ReloadCredentials")
            _refreshAutoStart()
        }
        root.refresh()
        // Re-query connection state after 3s to catch post-sync WebSocket updates
        delayedStateRefresh.restart()
    }

    Timer {
        id: delayedStateRefresh
        interval: 3000
        repeat: false
        onTriggered: _refreshConnectionState()
    }

    function _onDaemonLost() {
        root.isRunning = false
        root.messages = []
        root.unreadCount = 0
        root.connectionState = "disconnected"
    }

    Connections {
        target: dbusDaemon

        function onSignalReceived(signalName, args) {
            if (signalName === "NameOwnerChanged") {
                if (args[0] === "net.sailpush.Sailfish") {
                    // args[1] = old owner, args[2] = new owner
                    if (args[2].length > 0) {
                        // New owner appeared — daemon started
                        _onDaemonFound()
                    } else if (args[1].length > 0) {
                        // Old owner gone with no replacement — daemon stopped
                        _onDaemonLost()
                    }
                }
            }
        }
    }

    Connections {
        target: daemonInterface

        function onSignalReceived(signalName, args) {
            if (signalName === "ConnectionStateChanged") {
                root.connectionState = args[0]
                root.connectionStateUpdated(args[0])
            } else if (signalName === "CredentialsInvalidated") {
                root.credentialsInvalidated(args[0])
            } else if (signalName === "unreadCountChanged" || signalName === "MessageReceived") {
                root.refresh()
            }
        }
    }

    Timer {
        id: daemonCheckTimer
        interval: root.isRunning ? 10000 : 500
        running: true
        repeat: true
        onTriggered: _checkDaemonRunning()
    }

    function _checkDaemonRunning() {
        daemonInterface.typedCall("GetConnectionState", [],
            function(result) {
                _onDaemonFound()
                root.connectionState = result
            },
            function(error, message) {
                if (root.isRunning) {
                    _onDaemonLost()
                } else {
                    root.daemonRetryCount++
                    console.warn("Daemon check failed (retry " + root.daemonRetryCount + "): " + message)
                    if (root.daemonRetryCount >= 20) {
                        root.daemonError = qsTr("Cannot connect to background service")
                        daemonCheckTimer.stop()
                    }
                }
            }
        )
    }

    Component.onCompleted: {
        _checkDaemonRunning()
    }

    function triggerSync() {
        daemonInterface.call("TriggerSync")
        _delayedRefresh()
    }

    function markAsRead(messageId) {
        daemonInterface.call("MarkAsRead", [messageId])
        _delayedRefresh()
    }

    function markAllAsRead() {
        daemonInterface.call("MarkAllAsRead")
        _delayedRefresh()
    }

    function deleteMessage(messageId) {
        daemonInterface.call("DeleteMessage", [messageId])
        _delayedRefresh()
    }

    function acknowledgeEmergency(receipt) {
        daemonInterface.call("AcknowledgeEmergency", [receipt])
    }

    function openMessage(messageId) {
        daemonInterface.call("OpenMessage", [messageId])
    }

    function quitDaemon() {
        daemonInterface.call("Quit")
    }

    function reloadSettings(settingsMap) {
        if (settingsMap !== undefined) {
            daemonInterface.call("UpdateSettings", [settingsMap])
        } else {
            daemonInterface.call("ReloadSettings")
        }
    }

    function reloadCredentials() {
        daemonInterface.call("ReloadCredentials")
    }

    function refresh() {
        if (root.isRunning) {
            _refreshMessages()
            _refreshUnreadCount()
            _refreshConnectionState()
            _refreshDiagnostics()
        }
    }

    function _delayedRefresh() {
        refreshTimer.restart()
    }

    Timer {
        id: refreshTimer
        interval: 1000
        repeat: false
        onTriggered: root.refresh()
    }
}
