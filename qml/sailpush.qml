import QtQuick 2.0
import Sailfish.Silica 1.0
import "pages"

ApplicationWindow {
    id: app

    allowedOrientations: defaultAllowedOrientations
    cover: Qt.resolvedUrl("cover/CoverPage.qml")

    property bool hasCredentials: loginHelper ? loginHelper.hasCredentials : false
    property string pendingMessageId: ""
    property string lastNavigatedMessageId: ""

    DaemonConnector {
        id: daemonConnector
        visible: false

        onCredentialsInvalidated: {
            pendingMessageId = ""
            lastNavigatedMessageId = ""
            loginHelper.setMigrationReason(reason)
            navigateToLoginPage()
        }

        onOpenMessageRequested: {
            openMessageById(messageId)
        }

        onDaemonFound: {
            // Daemon just appeared on D-Bus — check for pending open message
            // that may have been written before we connected
            checkPendingOpenMessage()
        }
    }

    onHasCredentialsChanged: {
        if (hasCredentials && pageStack.currentPage != null) {
            daemonConnector.reloadCredentials()
            pageStack.replace(Qt.resolvedUrl("pages/MainPage.qml"), {rootDaemon: daemonConnector})
        }
    }

    // When app comes to foreground (e.g., lipstick activates it after notification tap),
    // refresh messages and check for pending open message. This is the reliable warm-start
    // path — the daemon writes the pending file synchronously, so it's guaranteed to exist
    // regardless of D-Bus signal delivery timing.
    Connections {
        target: Qt.application
        onStateChanged: {
            if (Qt.application.state === Qt.ApplicationActive && hasCredentials) {
                daemonConnector.refresh()
                pendingActivationCheck.restart()
            }
        }
    }

    Timer {
        id: pendingActivationCheck
        interval: 500
        repeat: false
        onTriggered: {
            if (pendingMessageId.length > 0) {
                // Already waiting for a message — onMessagesChanged will handle it
                // when messages finish refreshing from daemon
                return
            }
            checkPendingOpenMessage()
        }
    }

    Component.onCompleted: {
        if (hasCredentials) {
            pageStack.push(Qt.resolvedUrl("pages/MainPage.qml"), {rootDaemon: daemonConnector})
            checkPendingOpenMessage()
        } else {
            pageStack.push(Qt.resolvedUrl("pages/LoginPage.qml"))
        }
    }

    function checkPendingOpenMessage() {
        daemonConnector.checkPendingOpenMessage(function(messageId) {
            openMessageById(messageId)
        })
    }

    // Navigate to a specific message by ID. If messages aren't loaded yet,
    // wait briefly for them to arrive via D-Bus refresh.
    function openMessageById(messageId) {
        if (!hasCredentials) return

        var messageData = findMessageById(messageId)
        if (messageData) {
            navigateToMessage(messageData)
        } else {
            pendingMessageId = messageId
            pendingMessageTimer.start()
        }
    }

    function findMessageById(messageId) {
        if (!daemonConnector || !daemonConnector.messages) return null
        var messages = daemonConnector.messages
        for (var i = 0; i < messages.length; i++) {
            if (messages[i].id === messageId) {
                return messages[i]
            }
        }
        return null
    }

    function navigateToMessage(messageData) {
        if (lastNavigatedMessageId === messageData.id) {
            pendingMessageId = ""
            return
        }
        if (pageStack.busy) {
            Qt.callLater(navigateToMessage, messageData)
            return
        }
        lastNavigatedMessageId = messageData.id
        pendingMessageId = ""
        while (pageStack.depth > 1) {
            pageStack.pop(undefined, PageStackAction.Immediate)
        }
        if (pageStack.depth === 0) {
            pageStack.push(Qt.resolvedUrl("pages/MainPage.qml"), {rootDaemon: daemonConnector})
        }
        pageStack.push(Qt.resolvedUrl("pages/MessageDetailPage.qml"), {
            messageData: messageData,
            rootDaemon: daemonConnector
        })
    }

    Timer {
        id: pendingMessageTimer
        interval: 10000
        repeat: false
        onTriggered: {
            if (pendingMessageId.length > 0) {
                var messageData = findMessageById(pendingMessageId)
                if (messageData) {
                    pendingMessageId = ""
                    navigateToMessage(messageData)
                } else {
                    pendingMessageId = ""
                }
            }
        }
    }

    // When messages are refreshed (e.g., after daemon sync), check for pending message
    Connections {
        target: daemonConnector
        onMessagesChanged: {
            if (pendingMessageId.length > 0) {
                var messageData = findMessageById(pendingMessageId)
                if (messageData) {
                    navigateToMessage(messageData)
                }
            }
        }
    }

    function navigateToMainPage() {
        pageStack.clear()
        pageStack.push(Qt.resolvedUrl("pages/MainPage.qml"), {rootDaemon: daemonConnector})
    }

    function navigateToLoginPage() {
        pageStack.clear()
        pageStack.push(Qt.resolvedUrl("pages/LoginPage.qml"))
    }
}
