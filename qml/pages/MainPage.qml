import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

Page {
    id: page

    property var rootDaemon: null

    allowedOrientations: Orientation.All

    SilicaListView {
        id: listView
        anchors.fill: parent

        model: rootDaemon ? rootDaemon.messages : []

        header: Column {
            width: page.width

            PageHeader {
                title: appName
                description: rootDaemon && rootDaemon.unreadCount > 0 ? qsTr("%1 unread").arg(rootDaemon.unreadCount) : ""
            }

            ConnectionIndicator {
                state: rootDaemon ? rootDaemon.connectionState : "disconnected"
            }
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("Mark all read")
                onClicked: rootDaemon.markAllAsRead()
            }
            MenuItem {
                text: qsTr("Settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"), {rootDaemon: rootDaemon})
            }
        }

        delegate: MessageDelegate {
            width: listView.width
            messageData: modelData
            onClicked: {
                pageStack.push(Qt.resolvedUrl("MessageDetailPage.qml"), {
                    messageData: modelData,
                    rootDaemon: rootDaemon
                })
            }
        }

        ViewPlaceholder {
            enabled: rootDaemon ? rootDaemon.messages.length === 0 : true
            text: {
                if (rootDaemon && rootDaemon.daemonError.length > 0) return rootDaemon.daemonError
                if (rootDaemon && rootDaemon.isRunning) return qsTr("No Notifications")
                return qsTr("Waiting for daemon...")
            }
            hintText: {
                if (rootDaemon && rootDaemon.connectionState === "disconnected" && rootDaemon.diagnosticInfo.length > 0)
                    return rootDaemon.diagnosticInfo
                if (rootDaemon && rootDaemon.connectionState === "disconnected" && rootDaemon.isRunning)
                    return qsTr("Connection: disconnected\nPull down to sync")
                return ""
            }
        }

        VerticalScrollDecorator {}
    }
}
