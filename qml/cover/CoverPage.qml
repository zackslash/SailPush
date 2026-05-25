import QtQuick 2.0
import Sailfish.Silica 1.0


CoverBackground {
    Label {
        id: label
        anchors.centerIn: parent
        text: daemonConnector.unreadCount > 0 ? qsTr("%1 unread").arg(daemonConnector.unreadCount) : appName
    }

    CoverActionList {
        id: coverAction

        CoverAction {
            iconSource: "image://theme/icon-cover-refresh"
            onTriggered: daemonConnector.triggerSync()
        }
    }


}
