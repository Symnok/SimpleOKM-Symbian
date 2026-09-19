// SimpleOKM-Symbian - an Odnoklassniki messaging client for Symbian Anna/Belle.
// Copyright (C) 2026
// GPL-2.0-or-later; see the LICENSE file.
import QtQuick 1.1
import com.nokia.symbian 1.1

Page {
    id: page

    tools: ToolBarLayout {
        ToolButton { iconSource: "toolbar-back"; onClicked: Qt.quit() }
    }

    Flickable {
        id: flick
        anchors.fill: parent
        contentHeight: column.height + 2 * platformStyle.paddingLarge
        flickableDirection: Flickable.VerticalFlick
        clip: true

        Column {
            id: column
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: platformStyle.paddingLarge }
            spacing: platformStyle.paddingMedium

            Label {
                text: "SimpleOKM"
                font.pixelSize: platformStyle.fontSizeLarge * 1.5
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Label {
                text: qsTr("sign in to Odnoklassniki")
                color: platformStyle.colorNormalMid
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Item { width: 1; height: platformStyle.paddingLarge }

            Label { text: qsTr("login (phone or e-mail)"); font.pixelSize: platformStyle.fontSizeSmall }
            TextField {
                id: loginField
                width: parent.width
                text: app.savedLogin
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText | Qt.ImhEmailCharactersOnly
                placeholderText: qsTr("login")
                enabled: !app.busy
            }
            Label { text: qsTr("password"); font.pixelSize: platformStyle.fontSizeSmall }
            TextField {
                id: passwordField
                width: parent.width
                echoMode: TextInput.Password
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                placeholderText: qsTr("password")
                enabled: !app.busy
                Keys.onReturnPressed: signIn()
                Keys.onEnterPressed: signIn()
            }

            Button {
                text: app.busy ? qsTr("signing in...") : qsTr("sign in")
                width: parent.width
                enabled: !app.busy
                onClicked: signIn()
            }

            Label {
                id: errorLabel
                width: parent.width
                wrapMode: Text.Wrap
                text: app.loginError
                visible: text != ""
                color: app.verificationUrl != "" ? platformStyle.colorNormalLight : "#ff6b6b"
                font.pixelSize: platformStyle.fontSizeSmall
            }
            Button {
                text: qsTr("open verification in the browser")
                width: parent.width
                visible: app.verificationUrl != ""
                onClicked: app.openVerification()
            }

            Item { width: 1; height: platformStyle.paddingLarge }
            Label {
                width: parent.width
                wrapMode: Text.Wrap
                font.pixelSize: platformStyle.fontSizeSmall
                color: platformStyle.colorNormalMid
                text: app.sslSupported
                      ? qsTr("The password is sent once; afterwards a token kept on this phone is used.")
                      : qsTr("This Qt build has no TLS support - install the Qt TLS patch (nnproject.cc/qtls) first.")
            }
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: app.busy
        visible: app.busy
        width: platformStyle.graphicSizeLarge
        height: platformStyle.graphicSizeLarge
    }

    function signIn() {
        passwordField.closeSoftwareInputPanel()
        app.login(loginField.text, passwordField.text)
    }
}
