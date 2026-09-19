// SimpleOKM-Symbian - an Odnoklassniki messaging client for Symbian Anna/Belle.
// Copyright (C) 2026
// GPL-2.0-or-later; see the LICENSE file.
import QtQuick 1.1
import com.nokia.symbian 1.1

Page {
    id: page

    Component { id: chatPage; ChatPage {} }
    Component { id: newChatPage; NewChatPage {} }
    Component { id: settingsPage; SettingsPage {} }
    Component { id: aboutPage; AboutPage {} }

    tools: ToolBarLayout {
        ToolButton { iconSource: "toolbar-back"; onClicked: Qt.quit() }
        ToolButton { iconSource: "toolbar-add"; onClicked: pageStack.push(newChatPage) }
        ToolButton { iconSource: "toolbar-refresh"; enabled: !app.busy; onClicked: app.refreshConversations() }
        ToolButton { iconSource: "toolbar-menu"; onClicked: menu.open() }
    }

    Menu {
        id: menu
        MenuLayout {
            MenuItem { text: qsTr("Settings"); onClicked: pageStack.push(settingsPage) }
            MenuItem { text: qsTr("About"); onClicked: pageStack.push(aboutPage) }
            MenuItem { text: qsTr("Sign out"); onClicked: signOutDialog.open() }
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

    QueryDialog {
        id: deleteDialog
        property string conversationId
        property string title
        titleText: qsTr("Delete chat")
        message: qsTr("Delete \"%1\"?").arg(title)
        acceptButtonText: qsTr("Delete")
        rejectButtonText: qsTr("Cancel")
        onAccepted: app.deleteConversation(conversationId)
    }

    ContextMenu {
        id: contextMenu
        property int row: -1
        property variant item
        MenuLayout {
            MenuItem {
                text: qsTr("Open profile")
                visible: contextMenu.item ? contextMenu.item.peerUid != "" : false
                onClicked: app.openUrl("https://ok.ru/profile/" + contextMenu.item.peerUid)
            }
            MenuItem {
                text: qsTr("Delete chat")
                onClicked: {
                    deleteDialog.conversationId = contextMenu.item.conversationId
                    deleteDialog.title = contextMenu.item.title
                    deleteDialog.open()
                }
            }
        }
    }

    ListHeading {
        id: heading
        anchors { top: parent.top; left: parent.left; right: parent.right }
        ListItemText {
            anchors { fill: heading.paddingItem }
            role: "Heading"
            text: qsTr("Chats")
        }
        BusyIndicator {
            anchors { right: heading.paddingItem.right; verticalCenter: parent.verticalCenter }
            running: app.busy
            visible: app.busy
        }
    }

    ListView {
        id: list
        anchors { top: heading.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        model: app.conversations
        clip: true
        cacheBuffer: 400

        delegate: ListItem {
            id: item
            subItemIndicator: false

            Avatar {
                id: avatar
                anchors { left: item.paddingItem.left; verticalCenter: parent.verticalCenter }
                source: model.avatar
                name: model.title
                group: model.isChat
            }
            Column {
                anchors {
                    left: avatar.right; leftMargin: platformStyle.paddingMedium
                    right: rightColumn.left; rightMargin: platformStyle.paddingSmall
                    verticalCenter: parent.verticalCenter
                }
                ListItemText {
                    width: parent.width
                    role: "Title"
                    text: model.isSelf ? qsTr("Notes to myself") : model.title
                    elide: Text.ElideRight
                }
                ListItemText {
                    width: parent.width
                    role: "SubTitle"
                    text: model.isDraft ? qsTr("new chat") : model.preview
                    elide: Text.ElideRight
                    color: model.unread > 0 ? platformStyle.colorNormalLight : platformStyle.colorNormalMid
                }
            }
            Column {
                id: rightColumn
                anchors { right: item.paddingItem.right; verticalCenter: parent.verticalCenter }
                spacing: platformStyle.paddingSmall
                Label {
                    anchors.right: parent.right
                    text: model.timeText
                    font.pixelSize: platformStyle.fontSizeSmall
                    color: platformStyle.colorNormalMid
                }
                Rectangle {
                    anchors.right: parent.right
                    width: Math.max(badgeLabel.width + 12, 24)
                    height: 24
                    radius: 12
                    color: "#e8722a"
                    visible: model.unread > 0
                    Label {
                        id: badgeLabel
                        anchors.centerIn: parent
                        text: model.unread
                        font.pixelSize: platformStyle.fontSizeSmall
                        color: "white"
                    }
                }
            }

            onClicked: {
                app.chat.open(model.conversationId, model.unread)
                pageStack.push(chatPage)
            }
            onPressAndHold: {
                contextMenu.row = index
                contextMenu.item = app.conversations.get(index)
                contextMenu.open()
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
        visible: list.count == 0 && !app.busy
        text: app.listError != "" ? qsTr("Not updating: %1").arg(app.listError) : qsTr("No conversations yet. Tap + to start one.")
    }

    Label {
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right; margins: platformStyle.paddingSmall }
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: platformStyle.fontSizeSmall
        color: "#ff6b6b"
        elide: Text.ElideRight
        visible: app.listError != "" && list.count > 0
        text: qsTr("Not updating: %1").arg(app.listError)
    }

    onStatusChanged: {
        // Poll the list only while it is the visible page; the open chat polls itself.
        if (status == PageStatus.Active) {
            app.setListPolling(true)
            app.chat.close()
        } else if (status == PageStatus.Inactive) {
            app.setListPolling(false)
        }
    }
}
