import QtQuick 2.0
import Sailfish.Silica 1.0
import "pages"

ApplicationWindow {
    id: app

    allowedOrientations: defaultAllowedOrientations
    cover: Qt.resolvedUrl("cover/CoverPage.qml")

    property bool hasCredentials: loginHelper ? loginHelper.hasCredentials : false

    DaemonConnector {
        id: daemonConnector
        visible: false

        onCredentialsInvalidated: {
            loginHelper.setMigrationReason(reason)
            navigateToLoginPage()
        }
    }

    onHasCredentialsChanged: {
        if (hasCredentials && pageStack.currentPage != null) {
            daemonConnector.reloadCredentials()
            pageStack.replace(Qt.resolvedUrl("pages/MainPage.qml"), {rootDaemon: daemonConnector})
        }
    }

    Component.onCompleted: {
        if (hasCredentials) {
            pageStack.push(Qt.resolvedUrl("pages/MainPage.qml"), {rootDaemon: daemonConnector})
        } else {
            pageStack.push(Qt.resolvedUrl("pages/LoginPage.qml"))
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
