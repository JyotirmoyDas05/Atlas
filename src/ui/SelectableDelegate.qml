import QtQuick

// Reusable delegate base: provides selection/hover background and click handling
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
