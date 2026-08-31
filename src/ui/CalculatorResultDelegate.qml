import QtQuick
import QtQuick.Layouts

// Calculator result delegate — 90px tall, question ↔ arrow ↔ answer layout
SelectableDelegate {
    id: root
    height: 90

    required property string calcQuestion
    required property string calcQuestionUnit
    required property string calcAnswer
    required property string calcAnswerUnit

    Item {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16

        Column {
            id: leftColumn
            anchors.left: parent.left
            anchors.right: arrowContainer.left
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 4

            Text {
                width: parent.width
                text: root.calcQuestion
                color: root.selected ? Theme.listItemSelectionFg : Theme.foreground
                font.family: Theme.fontFamily
                font.pixelSize: Theme.regularFontSize * 1.5
                font.weight: Font.Medium
                elide: Text.ElideRight
                maximumLineCount: 1
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                width: parent.width
                text: root.calcQuestionUnit || "Expression"
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.smallerFontSize
                elide: Text.ElideRight
                maximumLineCount: 1
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Item {
            id: arrowContainer
            anchors.centerIn: parent
            width: 30
            height: 30

            Text {
                anchors.centerIn: parent
                text: "→"
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: 18
            }
        }

        Column {
            anchors.left: arrowContainer.right
            anchors.leftMargin: 12
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: 4

            Text {
                width: parent.width
                text: root.calcAnswer
                color: root.selected ? Theme.listItemSelectionFg : Theme.accent
                font.family: Theme.fontFamily
                font.pixelSize: Theme.regularFontSize * 1.5
                font.weight: Font.DemiBold
                elide: Text.ElideRight
                maximumLineCount: 1
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                width: parent.width
                text: root.calcAnswerUnit || "Result"
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.smallerFontSize
                elide: Text.ElideRight
                maximumLineCount: 1
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
