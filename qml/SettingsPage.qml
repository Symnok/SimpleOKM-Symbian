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

    SelectionDialog {
        id: languageDialog
        titleText: qsTr("App language")
        model: ListModel {
            ListElement { name: "System default"; code: "" }
            ListElement { name: "English"; code: "en" }
            ListElement { name: "Русский"; code: "ru" }
            ListElement { name: "Українська"; code: "uk" }
        }
        // The stock delegate shows modelData (a string list); this model has roles, and the
        // phone theme's dialog text is hard to read - so: our own rows, white on the dialog.
        delegate: Item {
            width: parent ? parent.width : 300
            height: (typeof privateStyle != "undefined") ? privateStyle.menuItemHeight : 56
            Rectangle { anchors.fill: parent; color: rowMouse.pressed ? "#3d5a80" : "transparent" }
            Label {
                anchors { left: parent.left; leftMargin: platformStyle.paddingLarge; right: parent.right; verticalCenter: parent.verticalCenter }
                text: (index == 0 ? qsTr("System default") : model.name) + (model.code == app.language ? "   *" : "")
                color: "white"
                elide: Text.ElideRight
            }
            MouseArea {
                id: rowMouse
                anchors.fill: parent
                onClicked: { languageDialog.selectedIndex = index; languageDialog.accept() }
            }
        }
        onAccepted: if (selectedIndex >= 0) app.language = model.get(selectedIndex).code
    }

    function languageName() {
        for (var i = 0; i < languageDialog.model.count; ++i)
            if (languageDialog.model.get(i).code == app.language)
                return i == 0 ? qsTr("System default") : languageDialog.model.get(i).name
        return qsTr("System default")
    }

    ListHeading {
        id: heading
        anchors { top: parent.top; left: parent.left; right: parent.right }
        ListItemText { anchors.fill: heading.paddingItem; role: "Heading"; text: qsTr("Settings") }
    }

    Flickable {
        anchors { top: heading.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        contentHeight: column.height
        clip: true

        Column {
            id: column
            width: parent.width

            ListItem {
                id: imagesItem
                ListItemText {
                    anchors { left: imagesItem.paddingItem.left; right: imagesSwitch.left; verticalCenter: parent.verticalCenter }
                    role: "Title"
                    text: qsTr("Load photos and avatars")
                    wrapMode: Text.Wrap
                }
                Switch {
                    id: imagesSwitch
                    anchors { right: imagesItem.paddingItem.right; verticalCenter: parent.verticalCenter }
                    checked: app.loadImages
                    onCheckedChanged: if (checked != app.loadImages) app.loadImages = checked
                }
                onClicked: imagesSwitch.checked = !imagesSwitch.checked
            }
            Label {
                width: parent.width - 2 * platformStyle.paddingLarge
                x: platformStyle.paddingLarge
                wrapMode: Text.Wrap
                font.pixelSize: platformStyle.fontSizeSmall
                color: platformStyle.colorNormalMid
                text: qsTr("Off, photos in chats are loaded only when tapped - useful on a slow or metered connection.")
            }
            Item { width: 1; height: platformStyle.paddingLarge }

            ListItem {
                id: languageItem
                subItemIndicator: true
                Column {
                    anchors { left: languageItem.paddingItem.left; right: languageItem.paddingItem.right; verticalCenter: parent.verticalCenter }
                    ListItemText {
                        width: parent.width
                        role: "Title"
                        text: qsTr("App language")
                    }
                    Label {
                        width: parent.width
                        text: languageName()
                        color: "white"
                        font.pixelSize: platformStyle.fontSizeSmall
                        elide: Text.ElideRight
                    }
                }
                onClicked: {
                    for (var i = 0; i < languageDialog.model.count; ++i)
                        if (languageDialog.model.get(i).code == app.language) languageDialog.selectedIndex = i
                    languageDialog.open()
                }
            }
            Item { width: 1; height: platformStyle.paddingLarge }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width - 2 * platformStyle.paddingLarge
                text: qsTr("Sign out")
                onClicked: signOutDialog.open()
            }
            Item { width: 1; height: platformStyle.paddingMedium }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width - 2 * platformStyle.paddingLarge
                text: qsTr("Exit")
                onClicked: Qt.quit()
            }
        }
    }

    QueryDialog {
        id: signOutDialog
        titleText: qsTr("Sign out")
        message: qsTr("Sign out? The saved token will be removed from this phone.")
        acceptButtonText: qsTr("Sign out")
        rejectButtonText: qsTr("Cancel")
        onAccepted: app.signOut()
    }
}
