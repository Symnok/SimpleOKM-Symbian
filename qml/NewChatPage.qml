// SimpleOKM-Symbian - an Odnoklassniki messaging client for Symbian Anna/Belle.
// Copyright (C) 2026
// GPL-2.0-or-later; see the LICENSE file.
//
// "New chat": pick a friend, open (or draft) the one-to-one with them.
import QtQuick 1.1
import com.nokia.symbian 1.1

Page {
    id: page

    Component { id: chatPage; ChatPage {} }

    tools: ToolBarLayout {
        ToolButton { iconSource: "toolbar-back"; onClicked: pageStack.pop() }
        ToolButton { iconSource: "toolbar-refresh"; enabled: !app.friends.loading; onClicked: app.friends.load() }
    }

    ListHeading {
        id: heading
        anchors { top: parent.top; left: parent.left; right: parent.right }
        ListItemText {
            anchors { fill: heading.paddingItem }
            role: "Heading"
            text: qsTr("New chat")
        }
        BusyIndicator {
            anchors { right: heading.paddingItem.right; verticalCenter: parent.verticalCenter }
            running: app.friends.loading
            visible: running
        }
    }

    ListView {
        id: list
        anchors { top: heading.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        model: app.friends
        clip: true

        delegate: ListItem {
            id: item
            Avatar {
                id: avatar
                anchors { left: item.paddingItem.left; verticalCenter: parent.verticalCenter }
                source: model.avatar
                name: model.name
            }
            ListItemText {
                anchors { left: avatar.right; leftMargin: platformStyle.paddingMedium; right: onlineLabel.left; rightMargin: platformStyle.paddingMedium; verticalCenter: parent.verticalCenter }
                role: "Title"
                text: model.name
                elide: Text.ElideRight
            }
            Label {
                id: onlineLabel
                anchors { right: item.paddingItem.right; verticalCenter: parent.verticalCenter }
                text: model.online ? qsTr("online") : ""
                font.pixelSize: platformStyle.fontSizeSmall
                color: "#6fcf6f"
            }
            onClicked: {
                var id = app.friends.openConversation(index)
                if (id != "") {
                    app.chat.open(id, 0)
                    pageStack.replace(chatPage)
                }
            }
        }
        ScrollDecorator { flickableItem: list }
    }

    Label {
        anchors.centerIn: list
        width: parent.width - 2 * platformStyle.paddingLarge
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        color: platformStyle.colorNormalMid
        visible: list.count == 0 && !app.friends.loading
        text: app.friends.error != "" ? app.friends.error : qsTr("No friends found.")
    }

    Component.onCompleted: if (app.friends.count == 0) app.friends.load()
}
