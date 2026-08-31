import QtQuick

// Reusable delegate base: selection/hover background, an accent bar on the
// selected row, and click handling.
Item {
    id: root

    property bool selected: false

    readonly property bool hovered: mouseArea.containsMouse

    default property alias contentData: contentItem.data

    signal clicked
    signal activated

    // Selection / hover background
    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: 6
        anchors.rightMargin: 6
        radius: 8
        color: {
            if (root.selected) return Theme.listItemSelectionBg;
            if (root.hovered)  return Theme.listItemHoverBg;
            return "transparent";
        }
        Behavior on color { ColorAnimation { duration: 80 } }
    }

    // Accent indicator: a slim rounded bar on the leading edge of the
    // selected row -- reads as "current" at a glance, independent of theme
    // contrast, and costs nothing when not selected.
    Rectangle {
        width: 3
        radius: 1.5
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 6
        height: parent.height * 0.5
        color: Theme.accent
        opacity: root.selected ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: 80 } }
    }

    Item {
        id: contentItem
        anchors.fill: parent
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
        onDoubleClicked: root.activated()
    }
}
