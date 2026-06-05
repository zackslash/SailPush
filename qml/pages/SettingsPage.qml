import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0

Page {
    id: page

    allowedOrientations: Orientation.All

    property bool systemNotifications: true
    property bool pollingFallback: true
    property int pollingIntervalIndex: 1
    property bool preventDeepSleep: false
    property var rootDaemon: null

    ConfigurationGroup {
        id: settings
        path: "/apps/com.zackslash/sailpush"
    }

    function _pushSettingsToDaemon() {
        if (rootDaemon) {
            var settingsMap = {
                "pollingFallback": page.pollingFallback,
                "pollingIntervalIndex": page.pollingIntervalIndex,
                "preventDeepSleep": page.preventDeepSleep,
                "systemNotifications": page.systemNotifications
            }
            rootDaemon.reloadSettings(settingsMap)
        }
    }

    Component.onCompleted: {
        pollingFallback = settings.value("pollingFallback", true)
        pollingIntervalIndex = settings.value("pollingIntervalIndex", 1)
        preventDeepSleep = settings.value("preventDeepSleep", false)
        systemNotifications = settings.value("systemNotifications", true)
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: page.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: qsTr("Settings")
            }

            SectionHeader {
                text: qsTr("Connection")
            }

            Label {
                x: Theme.horizontalPageMargin
                text: qsTr("Status: %1").arg(rootDaemon.connectionState)
                color: {
                    if (rootDaemon.connectionState === "ready" || rootDaemon.connectionState === "connected") return Theme.highlightColor
                    if (rootDaemon.connectionState === "error") return "#CC0000"
                    return Theme.secondaryColor
                }
            }

            Button {
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Sync Now")
                onClicked: rootDaemon.triggerSync()
            }

            SectionHeader {
                text: qsTr("Polling Fallback")
            }

            TextSwitch {
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Enable Polling Fallback")
                checked: page.pollingFallback
                onCheckedChanged: {
                    page.pollingFallback = checked
                    settings.setValue("pollingFallback", checked)
                    _pushSettingsToDaemon()
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                text: qsTr("If WebSocket disconnects for 30s, falls back to periodic polling for notifications.")
                color: Theme.secondaryColor
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeSmall
            }

            ComboBox {
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                label: qsTr("Polling Interval")
                menu: ContextMenu {
                    MenuItem { text: qsTr("1 minute"); onClicked: page.pollingIntervalIndex = 0 }
                    MenuItem { text: qsTr("5 minutes"); onClicked: page.pollingIntervalIndex = 1 }
                    MenuItem { text: qsTr("15 minutes"); onClicked: page.pollingIntervalIndex = 2 }
                    MenuItem { text: qsTr("30 minutes"); onClicked: page.pollingIntervalIndex = 3 }
                }
                currentIndex: page.pollingIntervalIndex
                onCurrentIndexChanged: {
                    page.pollingIntervalIndex = currentIndex
                    settings.setValue("pollingIntervalIndex", currentIndex)
                    _pushSettingsToDaemon()
                }
            }

            SectionHeader {
                text: qsTr("Deep Sleep")
            }

            TextSwitch {
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Prevent Deep Sleep During Sync")
                checked: page.preventDeepSleep
                onCheckedChanged: {
                    page.preventDeepSleep = checked
                    settings.setValue("preventDeepSleep", checked)
                    _pushSettingsToDaemon()
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Keeps CPU awake during message sync to ensure reliable delivery. Uses MCE keepalive API.")
                color: Theme.secondaryColor
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeSmall
            }

            Label {
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Note: For persistent WebSocket in deep sleep, run:\nmcetool --set-suspend-policy=early")
                color: Theme.secondaryColor
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeSmall
            }

            SectionHeader {
                text: qsTr("Notifications")
            }

            TextSwitch {
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                text: qsTr("System Notifications")
                checked: page.systemNotifications
                onCheckedChanged: {
                    page.systemNotifications = checked
                    settings.setValue("systemNotifications", checked)
                    _pushSettingsToDaemon()
                }
            }

            SectionHeader {
                text: qsTr("Background")
            }

            TextSwitch {
                id: autoStartSwitch
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Start on boot")
                description: qsTr("Automatically start the notification service at boot")
                checked: rootDaemon.autoStartEnabled
                onCheckedChanged: {
                    if (checked !== rootDaemon.autoStartEnabled) {
                        rootDaemon.toggleAutoStart()
                    }
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Automatically start the notification daemon when the device boots.")
                color: Theme.secondaryColor
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeSmall
            }

            SectionHeader {
                text: qsTr("Account")
            }

            Button {
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Logout")
                onClicked: pageStack.push(dialogComponent)
            }

            SectionHeader {
                text: qsTr("About")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                text: qsTr("%1 v%2").arg(appName).arg(appVersion)
                color: Theme.secondaryColor
            }

            Label {
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Unofficial Pushover Open Client for SailfishOS.\nNot released or supported by Pushover, LLC.")
                color: Theme.secondaryColor
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeExtraSmall
            }
        }
    }

    Component {
        id: dialogComponent
        Dialog {
            id: dialog

            onAccepted: {
                loginHelper.logout()
                rootDaemon.quitDaemon()
                pageStack.clear()
                pageStack.push(Qt.resolvedUrl("LoginPage.qml"))
            }

            Column {
                width: parent.width
                spacing: Theme.paddingLarge

                DialogHeader {
                    title: qsTr("Logout")
                    acceptText: qsTr("Logout")
                    cancelText: qsTr("Cancel")
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    wrapMode: Text.Wrap
                    color: Theme.secondaryHighlightColor
                    text: qsTr("This will clear your credentials and stop the background service. Are you sure?")
                }
            }
        }
    }
}
