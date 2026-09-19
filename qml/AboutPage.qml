// SimpleOKM-Symbian - an Odnoklassniki messaging client for Symbian Anna/Belle.
// Copyright (C) 2026
// GPL-2.0-or-later; see the LICENSE file.
import QtQuick 1.1
import com.nokia.symbian 1.1

Page {
    id: page

    tools: ToolBarLayout {
        ToolButton { iconSource: "toolbar-back"; onClicked: pageStack.pop() }
    }

    ListHeading {
        id: heading
        anchors { top: parent.top; left: parent.left; right: parent.right }
        ListItemText { anchors.fill: heading.paddingItem; role: "Heading"; text: qsTr("About") }
    }

    Flickable {
        anchors { top: heading.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        contentHeight: column.height + 2 * platformStyle.paddingLarge
        clip: true

        Column {
            id: column
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: platformStyle.paddingLarge }
            spacing: platformStyle.paddingMedium

            Label { text: "SimpleOKM"; font.pixelSize: platformStyle.fontSizeLarge * 1.3 }
            Label { text: qsTr("version %1").arg(app.version); color: platformStyle.colorNormalMid }
            Label {
                width: parent.width; wrapMode: Text.Wrap
                text: qsTr("An Odnoklassniki messaging client for Symbian Anna and Belle.")
            }
            Label {
                width: parent.width; wrapMode: Text.Wrap
                visible: app.myName != ""
                text: qsTr("Signed in as %1").arg(app.myName)
            }
            Label {
                width: parent.width; wrapMode: Text.Wrap
                font.pixelSize: platformStyle.fontSizeSmall
                color: platformStyle.colorNormalMid
                text: app.sslSupported
                      ? qsTr("TLS: this Qt build has OpenSSL support. Server roots (HARICA) are bundled with the app.")
                      : qsTr("TLS: NOT available in this Qt build. Install the Qt TLS 1.2 patch from nnproject.cc/qtls.")
            }
            Label {
                width: parent.width; wrapMode: Text.Wrap
                font.pixelSize: platformStyle.fontSizeSmall
                color: platformStyle.colorNormalMid
                text: qsTr("Derived from SimpleOKM (Android) and OKLumessenger (Windows Phone). GPL-2.0-or-later.")
            }
            Button {
                text: qsTr("Source on GitHub")
                onClicked: app.openUrl("https://github.com/Symnok/SimpleOKM-Symbian")
            }
        }
    }
}
