import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    allowedOrientations: Orientation.All

    property var messageData: ({})
    property var rootDaemon: null

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: page.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: messageData.display_name || messageData.title || messageData.app || qsTr("Message")
            }

            Row {
                x: Theme.horizontalPageMargin
                spacing: Theme.paddingSmall

                Image {
                    width: Theme.iconSizeMedium
                    height: Theme.iconSizeMedium
                    source: messageData.icon ? "https://api.pushover.net/icons/" + messageData.icon + ".png" : ""
                    fillMode: Image.PreserveAspectFit
                    visible: source != ""
                    cache: true
                }

                Column {
                    spacing: Theme.paddingSmall

                    Label {
                        text: messageData.app || ""
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.secondaryColor
                    }

                    Label {
                        text: {
                            if (messageData.date) {
                                var d = new Date(messageData.date * 1000)
                                return d.toLocaleString(Qt.locale(), "yyyy-MM-dd HH:mm:ss")
                            }
                            return ""
                        }
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.secondaryColor
                    }
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                text: messageData.html ? stripHtml(messageData.message || "") : (messageData.message || "")
                font.pixelSize: Theme.fontSizeMedium
                wrapMode: Text.WordWrap
            }

            Button {
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Open URL")
                visible: messageData.url && messageData.url !== ""
                onClicked: Qt.openUrlExternally(messageData.url)
            }

            Label {
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                text: messageData.url_title || messageData.url || ""
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                visible: messageData.url && messageData.url !== ""
                wrapMode: Text.WordWrap
            }

            Button {
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Acknowledge")
                visible: messageData.is_emergency && !messageData.acked
                onClicked: {
                    rootDaemon.acknowledgeEmergency(messageData.receipt)
                    pageStack.pop()
                }
            }

            RemorsePopup {
                id: remorse
            }

            Button {
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Delete")
                onClicked: remorse.execute(qsTr("Deleting"), function() {
                    rootDaemon.deleteMessage(messageData.id)
                    pageStack.pop()
                }, 1500)
            }
        }
    }

    Component.onCompleted: {
        if (messageData.id && rootDaemon) {
            rootDaemon.markAsRead(messageData.id)
        }
    }

    function stripHtml(html) {
        var text = html
        text = text.replace(/<br\s*\/?>/gi, "\n")
        text = text.replace(/<\/p>/gi, "\n\n")
        text = text.replace(/<[^>]*>/g, "")
        // Named entities
        text = text.replace(/&amp;/g, "&")
        text = text.replace(/&lt;/g, "<")
        text = text.replace(/&gt;/g, ">")
        text = text.replace(/&quot;/g, "\"")
        text = text.replace(/&#39;|&apos;/g, "'")
        text = text.replace(/&nbsp;/g, " ")
        text = text.replace(/&copy;/g, "\u00A9")
        text = text.replace(/&mdash;/g, "\u2014")
        text = text.replace(/&ndash;/g, "\u2013")
        text = text.replace(/&hellip;/g, "\u2026")
        // Numeric entities: &#123; and &#x1F600;
        text = text.replace(/&#x([0-9a-fA-F]+);/g, function(match, hex) {
            return String.fromCharCode(parseInt(hex, 16))
        })
        text = text.replace(/&#(\d+);/g, function(match, dec) {
            return String.fromCharCode(parseInt(dec, 10))
        })
        return text
    }
}
