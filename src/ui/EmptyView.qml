import QtQuick
import QtQuick.Layouts

Item {
    id: root

    property string title: "No results"
    property string description: ""

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(380, parent.width)
        spacing: 12

        Text {
            visible: root.title !== ""
            text: root.title
            color: Theme.foreground
            font.family: Theme.fontFamily
            font.pixelSize: Theme.regularFontSize
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }

        Text {
            visible: root.description !== ""
            text: root.description
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.smallerFontSize
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }
    }
}
