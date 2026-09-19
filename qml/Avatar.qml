// SimpleOKM-Symbian - an Odnoklassniki messaging client for Symbian Anna/Belle.
// Copyright (C) 2026
// GPL-2.0-or-later; see the LICENSE file.
//
// A list avatar: the picture when there is one (and images are enabled), otherwise a tile
// with the first letter of the name.
import QtQuick 1.1
import com.nokia.symbian 1.1

Item {
    id: root
    property string source
    property string name
    property bool group: false
    width: platformStyle.graphicSizeLarge
    height: platformStyle.graphicSizeLarge

    Rectangle {
        anchors.fill: parent
        radius: 4
        color: group ? "#4a6b3a" : "#3b5f83"
        visible: !picture.visible || picture.status != Image.Ready
        Label {
            anchors.centerIn: parent
            text: group ? "#" : (name.length > 0 ? name.charAt(0).toUpperCase() : "?")
            font.pixelSize: platformStyle.fontSizeLarge
            color: "white"
        }
    }
    Image {
        id: picture
        anchors.fill: parent
        source: (app.loadImages && root.source != "") ? root.source : ""
        visible: source != ""
        asynchronous: true
        smooth: true
        sourceSize.width: width
        sourceSize.height: height
        fillMode: Image.PreserveAspectCrop
        clip: true
    }
}
