import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    allowedOrientations: Orientation.All

    property alias email: emailField.text
    property alias password: passwordField.text
    property alias twofa: twofaField.text

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: page.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: qsTr("Login")
            }

            Item {
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                height: migrationBanner.height + 2 * Theme.paddingMedium
                visible: loginHelper.migrationReason.length > 0

                Rectangle {
                    anchors.fill: parent
                    color: Theme.rgba(Theme.highlightBackgroundColor, 0.1)
                    radius: Theme.paddingSmall
                }

                Label {
                    id: migrationBanner
                    x: Theme.paddingSmall
                    y: Theme.paddingMedium
                    width: parent.width - 2 * Theme.paddingSmall
                    text: loginHelper.migrationReason
                    color: Theme.highlightColor
                    wrapMode: Text.WordWrap
                    font.pixelSize: Theme.fontSizeSmall
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Enter your Pushover account credentials to get started.")
                color: Theme.secondaryColor
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeSmall
            }

            TextField {
                id: emailField
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                label: qsTr("Email")
                inputMethodHints: Qt.ImhEmailCharactersOnly
            }

            TextField {
                id: passwordField
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                label: qsTr("Password")
                echoMode: TextInput.Password
            }

            TextField {
                id: twofaField
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                label: qsTr("Two-Factor Code")
                placeholderText: qsTr("Enter if required")
                visible: loginHelper.needsTwoFactor
            }

            Label {
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                text: loginHelper.errorString
                color: "#CC0000"
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeSmall
                visible: loginHelper.errorString.length > 0
            }

            Button {
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                text: loginHelper.loggingIn ? qsTr("Logging in...") :
                      loginHelper.registering ? qsTr("Registering device...") :
                      loginHelper.needsTwoFactor ? qsTr("Verify") : qsTr("Login")
                enabled: !loginHelper.loggingIn && !loginHelper.registering
                onClicked: {
                    loginHelper.login(emailField.text, passwordField.text, twofaField.text)
                }
            }

            Button {
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Sign up at pushover.net")
                visible: !loginHelper.loggingIn && !loginHelper.registering
                onClicked: Qt.openUrlExternally("https://pushover.net/signup")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                text: qsTr("This is an unofficial client. Not released or supported by Pushover, LLC.")
                color: Theme.secondaryHighlightColor
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeExtraSmall
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
