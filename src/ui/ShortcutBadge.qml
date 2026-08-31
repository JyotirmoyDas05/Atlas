import QtQuick

// Each token is an object: { text: "⌘", icon: "" } or a plain string
Item {
    id: root

    property var tokens: []
    property color contentColor: Theme.textMuted

    readonly property color _surfaceColor: Qt.rgba(root.contentColor.r, root.contentColor.g, root.contentColor.b, 0.10)
    readonly property color _borderColor:  Qt.rgba(root.contentColor.r, root.contentColor.g, root.contentColor.b, 0.18)

    implicitWidth:  tokenRow.implicitWidth
    implicitHeight: tokenRow.implicitHeight

    Row {
        id: tokenRow
        spacing: 4

        Repeater {
            model: root.tokens || []

            delegate: Item {
                required property var modelData

                // Accept plain strings or {text, icon} objects
                readonly property string tokenText: (typeof modelData === "string") ? modelData : (modelData["text"] || "")
                readonly property bool compact: tokenText.length <= 2

                implicitHeight: 20
                implicitWidth:  Math.max(compact ? implicitHeight : 0, label.implicitWidth + (compact ? 10 : 12))

                Rectangle {
                    anchors.fill: parent
                    radius: 5
                    color: root._surfaceColor
                    border.width: 1
                    border.color: root._borderColor
                }

                Text {
                    id: label
                    anchors.centerIn: parent
                    text: tokenText
                    color: root.contentColor
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.smallerFontSize - 1
                    font.weight: Font.Medium
                }
            }
        }
    }
}
