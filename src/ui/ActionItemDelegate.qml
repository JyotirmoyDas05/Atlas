import QtQuick
import QtQuick.Controls

// Action panel item row — used inside ActionPanelPopover list
Item {
    id: root
    height: 34

    required property string actionTitle
    required property var actionShortcut   // array of token objects
    property bool selected: false
    property bool isDanger: false

    readonly property bool hovered: mouseArea.containsMouse

    signal activated

    // Selection / hover background
    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: 6
        anchors.rightMargin: 6
        radius: 6
        color: root.selected ? Theme.listItemSelectionBg : (root.hovered ? Theme.listItemHoverBg : "transparent")
        Behavior on color { ColorAnimation { duration: 80 } }
    }

    Row {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 0

        Text {
            width: parent.width - shortcut.implicitWidth - 8
            anchors.verticalCenter: parent.verticalCenter
            text: root.actionTitle
            color: root.isDanger ? "#f43f5e" : (root.selected ? Theme.listItemSelectionFg : Theme.foreground)
            font.family: Theme.fontFamily
            font.pixelSize: Theme.regularFontSize
            elide: Text.ElideRight
        }

        ShortcutBadge {
            id: shortcut
            tokens: root.actionShortcut
            contentColor: root.selected ? Theme.listItemSelectionFg : Theme.textMuted
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.activated()
    }
}
