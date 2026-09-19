// SimpleOKM-Symbian - an Odnoklassniki messaging client for Symbian Anna/Belle.
// Copyright (C) 2026
// GPL-2.0-or-later; see the LICENSE file.
//
// The window: a page stack that follows app.state (starting -> login -> ready), and the
// notice banner every page shares.
import QtQuick 1.1
import com.nokia.symbian 1.1
import com.nokia.extras 1.1

PageStackWindow {
    id: window
    showStatusBar: true
    showToolBar: true
    platformSoftwareInputPanelEnabled: true
    initialPage: startPage

    Page {
        id: startPage
        BusyIndicator {
            anchors.centerIn: parent
            running: true
            width: platformStyle.graphicSizeLarge
            height: platformStyle.graphicSizeLarge
        }
        Label {
            anchors { top: parent.top; topMargin: parent.height / 4; horizontalCenter: parent.horizontalCenter }
            text: "SimpleOKM"
            font.pixelSize: platformStyle.fontSizeLarge * 1.5
        }
        tools: ToolBarLayout {
            ToolButton { iconSource: "toolbar-back"; onClicked: Qt.quit() }
        }
    }

    Component { id: loginPage; LoginPage {} }
    Component { id: conversationsPage; ConversationsPage {} }

    function route() {
        if (app.state == "login") {
            pageStack.clear()
            pageStack.push(loginPage)
        } else if (app.state == "ready") {
            pageStack.clear()
            pageStack.push(conversationsPage)
        }
    }

    Connections {
        target: app
        onStateChanged: route()
        onNoticeChanged: {
            if (app.notice != "") {
                banner.text = app.notice
                banner.open()
            }
        }
    }

    Component.onCompleted: route()

    InfoBanner {
        id: banner
        timeout: 4000
        onClicked: app.clearNotice()
    }
}
