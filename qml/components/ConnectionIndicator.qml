import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: indicator

    property string state: "disconnected"

    height: Theme.itemSizeExtraSmall
    width: parent ? parent.width : Screen.width

    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: Theme.paddingSmall

        Rectangle {
            width: Math.round(Theme.itemSizeExtraSmall / 2)
            height: Math.round(Theme.itemSizeExtraSmall / 2)
            radius: width / 2
            opacity: 0.8
            color: {
                if (indicator.state === "ready") return "#00CC00"
                if (indicator.state === "connecting") return Theme.highlightColor
                if (indicator.state === "error") return "#CC0000"
                if (indicator.state === "session_closed") return "#CC0000"
                return Theme.secondaryColor
            }
        }

        Label {
            text: {
                if (indicator.state === "ready") return qsTr("Connected")
                if (indicator.state === "connecting") return qsTr("Connecting...")
                if (indicator.state === "error") return qsTr("Error")
                if (indicator.state === "session_closed") return qsTr("Session closed")
                return qsTr("Disconnected")
            }
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
