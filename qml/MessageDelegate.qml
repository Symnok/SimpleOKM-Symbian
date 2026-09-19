// SimpleOKM-Symbian - an Odnoklassniki messaging client for Symbian Anna/Belle.
// Copyright (C) 2026
// GPL-2.0-or-later; see the LICENSE file.
//
// One message bubble: date separator, sender (group chats), quoted reply, text, and an
// inline photo / a video or voice tile / a link tile. Outgoing on the right, incoming on
// the left, system messages centred.
import QtQuick 1.1
import com.nokia.symbian 1.1

Item {
    id: root
    signal pressAndHold
    signal imageClicked
    signal mediaClicked(string url)

    property int maxBubbleWidth: width * 0.8
    property bool showPhoto: app.loadImages

    height: column.height + platformStyle.paddingSmall

    Column {
        id: column
        anchors { left: parent.left; right: parent.right }
        spacing: platformStyle.paddingSmall

        // Date separator
        Item {
            width: parent.width
            height: model.showDate ? dateLabel.height + platformStyle.paddingMedium : 0
            visible: model.showDate
            Label {
                id: dateLabel
                anchors.centerIn: parent
                text: model.dateText
                font.pixelSize: platformStyle.fontSizeSmall
                color: platformStyle.colorNormalMid
            }
        }

        // System message
        Label {
            width: parent.width - 2 * platformStyle.paddingLarge
            anchors.horizontalCenter: parent.horizontalCenter
            visible: model.isSystem
            text: model.body
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: platformStyle.fontSizeSmall
            color: platformStyle.colorNormalMid
        }

        // Bubble
        Rectangle {
            id: bubble
            visible: !model.isSystem
            // The width of what is actually painted inside: a photo or a tile takes the
            // full content width, text takes its painted (wrapped) width.
            property real innerWidth: Math.max(
                (model.hasImage || model.hasVideo || model.hasAudio || model.hasLink) ? contentColumn.width : 0,
                model.showSender ? senderLabel.paintedWidth : 0,
                model.replyText != "" ? quote.paintedWidth + 3 + platformStyle.paddingSmall : 0,
                model.body != "" ? bodyLabel.paintedWidth : 0,
                timeRow.width)
            width: Math.min(maxBubbleWidth, innerWidth + 2 * platformStyle.paddingMedium)
            height: contentColumn.height + timeRow.height + 2 * platformStyle.paddingMedium + platformStyle.paddingSmall
            radius: 8
            color: model.failed ? "#6b2b2b" : (model.out ? "#1f5e8a" : "#3a3a3a")
            opacity: model.pending ? 0.6 : 1
            anchors { right: model.out ? parent.right : undefined; left: model.out ? undefined : parent.left; margins: platformStyle.paddingMedium }

            MouseArea {
                anchors.fill: parent
                onPressAndHold: root.pressAndHold()
            }

            Column {
                id: contentColumn
                anchors { left: parent.left; top: parent.top; margins: platformStyle.paddingMedium }
                width: maxBubbleWidth - 2 * platformStyle.paddingMedium
                spacing: platformStyle.paddingSmall

                Label {
                    id: senderLabel
                    visible: model.showSender
                    text: model.senderName
                    font.pixelSize: platformStyle.fontSizeSmall
                    font.bold: true
                    color: "#f0a060"
                }

                // Quoted reply
                Row {
                    visible: model.replyText != ""
                    width: parent.width
                    spacing: platformStyle.paddingSmall
                    Rectangle { width: 3; height: quote.height; color: "#e8722a" }
                    Label {
                        id: quote
                        width: parent.width - 3 - platformStyle.paddingSmall
                        text: model.replyText
                        elide: Text.ElideRight
                        maximumLineCount: 2
                        wrapMode: Text.Wrap
                        font.pixelSize: platformStyle.fontSizeSmall
                        color: platformStyle.colorNormalMid
                    }
                }

                // Photo
                Item {
                    visible: model.hasImage
                    width: parent.width
                    height: model.hasImage ? (photo.status == Image.Ready ? photo.height : 120) : 0
                    Rectangle {
                        anchors.fill: parent
                        color: "#222222"
                        radius: 4
                        visible: photo.status != Image.Ready
                        Label {
                            anchors.centerIn: parent
                            text: photo.status == Image.Loading ? qsTr("loading...")
                                : (photo.status == Image.Error ? qsTr("photo unavailable") : qsTr("tap to load photo"))
                            font.pixelSize: platformStyle.fontSizeSmall
                            color: platformStyle.colorNormalMid
                        }
                    }
                    Image {
                        id: photo
                        width: parent.width
                        source: (model.hasImage && root.showPhoto) ? model.imageUrl : ""
                        asynchronous: true
                        smooth: true
                        fillMode: Image.PreserveAspectFit
                        sourceSize.width: parent.width
                        visible: status == Image.Ready
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (!root.showPhoto) root.showPhoto = true
                            else if (photo.status == Image.Ready) root.imageClicked()
                        }
                        onPressAndHold: root.pressAndHold()
                    }
                }

                // Video / voice / link tile
                Rectangle {
                    visible: model.hasVideo || model.hasAudio || model.hasLink
                    width: parent.width
                    height: tileLabel.height + 2 * platformStyle.paddingMedium
                    radius: 4
                    color: "#00000040"
                    Label {
                        id: tileLabel
                        anchors { left: parent.left; right: parent.right; margins: platformStyle.paddingMedium; verticalCenter: parent.verticalCenter }
                        text: (model.hasVideo ? qsTr("Video") : (model.hasAudio ? qsTr("Voice message") : qsTr("Link: %1").arg(model.attachmentTitle)))
                              + (model.attachmentDuration != "" ? " (" + model.attachmentDuration + ")" : "")
                        wrapMode: Text.Wrap
                        font.pixelSize: platformStyle.fontSizeSmall
                        color: platformStyle.colorNormalLink
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.mediaClicked(model.hasLink ? model.linkUrl : model.mediaUrl)
                        onPressAndHold: root.pressAndHold()
                    }
                }

                Label {
                    id: bodyLabel
                    visible: model.body != ""
                    width: parent.width
                    text: model.body
                    wrapMode: Text.Wrap
                    color: "white"
                }
            }

            Row {
                id: timeRow
                anchors { right: parent.right; bottom: parent.bottom; margins: platformStyle.paddingSmall }
                spacing: platformStyle.paddingSmall
                Label {
                    text: (model.edited ? qsTr("edited") + ", " : "") + model.timeText
                    font.pixelSize: platformStyle.fontSizeSmall * 0.85
                    color: "#c0c0c0"
                }
                Label {
                    visible: model.out
                    text: model.failed ? "!" : (model.pending ? "..." : "")
                    font.pixelSize: platformStyle.fontSizeSmall * 0.85
                    color: model.failed ? "#ff9b9b" : "#c0c0c0"
                }
            }
        }
    }
}
