import QtQuick

Item {
    id: root
    height: 30

    required property string text
    property real leftPadding: 16

    Text {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: root.leftPadding
        anchors.rightMargin: root.leftPadding
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 6
        text: root.text.toUpperCase()
        color: Theme.textMuted
        font.family: Theme.fontFamily
        font.pixelSize: Theme.smallerFontSize - 1
        font.weight: Font.DemiBold
        font.letterSpacing: 0.6
        elide: Text.ElideRight
    }
}
