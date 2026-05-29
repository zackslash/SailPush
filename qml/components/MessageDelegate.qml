import QtQuick 2.0
import Sailfish.Silica 1.0
import "../js/HtmlUtils.js" as HtmlUtils

ListItem {
    id: delegate

    property var messageData: ({})

    contentHeight: column.height + Theme.paddingLarge * 1.5
    width: parent ? parent.width : Screen.width

    ListView.onRemove: animateRemoval(delegate)

    menu: Component {
        ContextMenu {
            MenuItem {
                text: qsTr("Delete")
                onClicked: {
                    var msgId = messageData.id
                    var daemon = rootDaemon
                    delegate.remorseDelete(function() {
                        daemon.deleteMessage(msgId)
                    }, 2000)
                }
            }
        }
    }

    Rectangle {
        id: unreadDot
        width: Theme.paddingSmall
        height: Theme.paddingSmall
        radius: width / 2
        color: Theme.highlightColor
        anchors {
            left: parent.left
            leftMargin: Theme.paddingSmall
            verticalCenter: parent.verticalCenter
        }
        visible: !messageData.read
    }

    Column {
        id: column
        x: unreadDot.visible ? unreadDot.x + unreadDot.width + Theme.paddingSmall : Theme.paddingLarge
        y: Theme.paddingMedium
        width: delegate.width - x - Theme.paddingLarge
        spacing: Theme.paddingSmall

        Row {
            spacing: Theme.paddingSmall

            Image {
                width: Theme.iconSizeSmall
                height: Theme.iconSizeSmall
                source: messageData.icon ? "https://api.pushover.net/icons/" + messageData.icon + ".png" : ""
                fillMode: Image.PreserveAspectFit
                anchors.verticalCenter: parent.verticalCenter
                cache: true
            }

            Label {
                text: messageData.display_name || messageData.title || messageData.app || ""
                font.pixelSize: Theme.fontSizeMedium
                font.bold: !messageData.read
                color: {
                    if (messageData.priority >= 2) return "#CC0000"
                    if (messageData.priority >= 1) return Theme.highlightColor
                    if (messageData.priority <= -2) return Theme.secondaryColor
                    return delegate.highlighted ? Theme.highlightColor : Theme.primaryColor
                }
                truncationMode: TruncationMode.Fade
            }
        }

        Label {
            text: messageData.html ? HtmlUtils.stripHtml(messageData.message || "") : (messageData.message || "")
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.secondaryColor
            truncationMode: TruncationMode.Fade
            maximumLineCount: 2
        }

        Label {
            text: {
                if (messageData.date) {
                    var now = Date.now() / 1000
                    var diff = now - messageData.date
                    if (diff < 60) return qsTr("Just now")
                    if (diff < 3600) return qsTr("%1m ago").arg(Math.floor(diff / 60))
                    if (diff < 86400) return qsTr("%1h ago").arg(Math.floor(diff / 3600))
                    var d = new Date(messageData.date * 1000)
                    return d.toLocaleDateString(Qt.locale())
                }
                return ""
            }
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryHighlightColor
        }
    }
}
