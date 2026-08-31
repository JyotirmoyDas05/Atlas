import QtQuick

Item {
    id: root

    required property string text
    property color contentColor: Theme.textMuted

    readonly property color _surfaceColor: Qt.rgba(root.contentColor.r, root.contentColor.g, root.contentColor.b, 0.08)
    readonly property color _borderColor:  Qt.rgba(root.contentColor.r, root.contentColor.g, root.contentColor.b, 0.14)

    implicitWidth:  body.implicitWidth
    implicitHeight: body.implicitHeight
    width:  implicitWidth
    height: implicitHeight

    Item {
        id: body
        implicitHeight: 20
        implicitWidth:  badgeLabel.implicitWidth + 12
        anchors.fill: parent

        Rectangle {
            anchors.fill: parent
            radius: 6
            color: root._surfaceColor
            border.width: 1
            border.color: root._borderColor
        }

        Text {
            id: badgeLabel
            anchors.centerIn: parent
            text: root.text
            color: root.contentColor
            font.family: Theme.fontFamily
            font.pixelSize: Theme.smallerFontSize - 1
            font.weight: Font.Medium
            maximumLineCount: 1
        }
    }
}
