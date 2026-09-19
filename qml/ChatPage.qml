// SimpleOKM-Symbian - an Odnoklassniki messaging client for Symbian Anna/Belle.
// Copyright (C) 2026
// GPL-2.0-or-later; see the LICENSE file.
//
// One conversation: the message list (oldest at the top, newest at the bottom), a reply bar
// and the composer. Long-press a bubble for reply / copy / save / delete.
import QtQuick 1.1
import com.nokia.symbian 1.1

Page {
    id: page
    property variant chat: app.chat

    Component { id: imagePage; ImagePage {} }

    tools: ToolBarLayout {
        ToolButton { iconSource: "toolbar-back"; onClicked: pageStack.pop() }
        ToolButton {
            iconSource: "toolbar-add"
            enabled: !chat.isSelf && chat.conversationId != ""
            onClicked: attachPhoto()
        }
        ToolButton { iconSource: "toolbar-refresh"; enabled: !chat.loading; onClicked: chat.refresh() }
        ToolButton { iconSource: "toolbar-menu"; onClicked: menu.open() }
    }

    Menu {
        id: menu
        MenuLayout {
            MenuItem {
                text: qsTr("Load earlier messages")
                enabled: chat.hasMore && !chat.loadingEarlier
                onClicked: chat.loadEarlier()
            }
            MenuItem {
                text: qsTr("Open profile")
                visible: peerUid() != ""
                onClicked: app.openUrl("https://ok.ru/profile/" + peerUid())
            }
        }
    }

    function peerUid() {
        var i = app.conversations.indexOf(chat.conversationId)
        return i >= 0 ? app.conversations.get(i).peerUid : ""
    }

    function attachPhoto() {
        var path = app.pickImage()
        if (path != "") {
            chat.sendPhotoFile(path, composer.text)
            composer.text = ""
        }
    }

    ContextMenu {
        id: contextMenu
        property int row: -1
        property variant item
        MenuLayout {
            MenuItem {
                text: qsTr("Reply")
                visible: contextMenu.item ? (contextMenu.item.messageId != "" && !contextMenu.item.isSystem) : false
                onClicked: { chat.beginReply(contextMenu.row); composer.forceActiveFocus() }
            }
            MenuItem {
                text: qsTr("Copy text")
                visible: contextMenu.item ? contextMenu.item.body != "" : false
                onClicked: app.copyText(contextMenu.item.body)
            }
            MenuItem {
                text: qsTr("Save image")
                visible: contextMenu.item ? contextMenu.item.hasImage : false
                onClicked: app.saveImage(contextMenu.item.fullImageUrl)
            }
            MenuItem {
                text: qsTr("Open in browser")
                visible: contextMenu.item ? (contextMenu.item.hasVideo || contextMenu.item.hasAudio || contextMenu.item.hasLink) : false
                onClicked: app.openUrl(contextMenu.item.hasLink ? contextMenu.item.linkUrl : contextMenu.item.mediaUrl)
            }
            MenuItem {
                text: qsTr("Retry")
                visible: contextMenu.item ? contextMenu.item.failed : false
                onClicked: chat.retry(contextMenu.row)
            }
            MenuItem {
                text: qsTr("Delete")
                visible: contextMenu.item ? (contextMenu.item.canDelete || contextMenu.item.failed) : false
                onClicked: chat.deleteMessage(contextMenu.row)
            }
        }
    }

    // -- header --
    ListHeading {
        id: heading
        anchors { top: parent.top; left: parent.left; right: parent.right }
        ListItemText {
            anchors { fill: heading.paddingItem }
            role: "Heading"
            text: chat.isSelf ? qsTr("Notes to myself") : chat.title
            elide: Text.ElideRight
        }
        BusyIndicator {
            anchors { right: heading.paddingItem.right; verticalCenter: parent.verticalCenter }
            running: chat.loading || chat.loadingEarlier
            visible: running
        }
    }

    // -- messages --
    ListView {
        id: list
        anchors { top: heading.bottom; left: parent.left; right: parent.right; bottom: replyBar.top }
        model: chat
        clip: true
        spacing: platformStyle.paddingSmall
        cacheBuffer: 600

        header: Item {
            width: list.width
            height: chat.hasMore ? loadEarlierButton.height + 2 * platformStyle.paddingMedium : platformStyle.paddingSmall
            Button {
                id: loadEarlierButton
                anchors.centerIn: parent
                visible: chat.hasMore
                enabled: !chat.loadingEarlier
                text: chat.loadingEarlier ? qsTr("Loading...") : qsTr("Load earlier")
                onClicked: chat.loadEarlier()
            }
        }

        delegate: MessageDelegate {
            width: list.width
            onPressAndHold: {
                contextMenu.row = index
                contextMenu.item = chat.get(index)
                contextMenu.open()
            }
            onImageClicked: {
                pageStack.push(imagePage, { source: fullImageUrl, previewSource: imageUrl })
            }
            onMediaClicked: app.openUrl(url)
        }

        ScrollDecorator { flickableItem: list }

        function scrollToEnd() {
            if (count > 0) positionViewAtEnd()
        }
    }

    Connections {
        target: chat
        onLoadingChanged: {
            if (!chat.loading && chat.count > 0 && !chat.loadingEarlier) {
                if (chat.firstUnreadRow >= 0) list.positionViewAtIndex(chat.firstUnreadRow, ListView.Beginning)
                else list.scrollToEnd()
            }
        }
        onNewMessagesArrived: list.scrollToEnd()
        onCountChanged: {
            // Our own just-sent message: keep the view at the bottom.
            if (list.atYEnd || list.contentHeight < list.height) list.scrollToEnd()
        }
    }

    Label {
        anchors.centerIn: list
        width: parent.width - 2 * platformStyle.paddingLarge
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        color: platformStyle.colorNormalMid
        visible: list.count == 0 && !chat.loading
        text: chat.isSelf
              ? qsTr("OK's messaging API cannot read the chat with yourself. Open it in the browser instead.")
              : (chat.error != "" ? chat.error : qsTr("No messages yet."))
    }

    // -- reply bar --
    Rectangle {
        id: replyBar
        anchors { left: parent.left; right: parent.right; bottom: composerRow.top }
        height: chat.replyToId != "" ? replyLabel.height + 2 * platformStyle.paddingSmall : 0
        visible: chat.replyToId != ""
        color: "#222f3d"
        Rectangle { width: 3; height: parent.height; color: "#e8722a" }
        Label {
            id: replyLabel
            anchors { left: parent.left; leftMargin: platformStyle.paddingMedium; right: cancelReply.left; verticalCenter: parent.verticalCenter }
            text: qsTr("Reply: ") + chat.replyToText
            elide: Text.ElideRight
            font.pixelSize: platformStyle.fontSizeSmall
        }
        ToolButton {
            id: cancelReply
            anchors { right: parent.right; verticalCenter: parent.verticalCenter }
            text: "x"
            flat: true
            width: 40
            onClicked: chat.cancelReply()
        }
    }

    // -- composer --
    Item {
        id: composerRow
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: chat.isSelf ? 0 : composer.height + 2 * platformStyle.paddingSmall
        visible: !chat.isSelf

        TextArea {
            id: composer
            anchors {
                left: parent.left; leftMargin: platformStyle.paddingSmall
                right: sendButton.left; rightMargin: platformStyle.paddingSmall
                verticalCenter: parent.verticalCenter
            }
            placeholderText: qsTr("message")
            wrapMode: TextEdit.Wrap
            platformMaxImplicitHeight: 120
        }
        Button {
            id: sendButton
            anchors { right: parent.right; rightMargin: platformStyle.paddingSmall; verticalCenter: parent.verticalCenter }
            width: 80
            text: qsTr("Send")
            enabled: composer.text.length > 0 && chat.conversationId != ""
            onClicked: {
                chat.send(composer.text)
                composer.text = ""
            }
        }
    }

    onStatusChanged: {
        if (status == PageStatus.Active) chat.setPolling(true)
        else if (status == PageStatus.Inactive) chat.setPolling(false)
    }
}
