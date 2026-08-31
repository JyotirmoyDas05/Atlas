import QtQuick
import QtQuick.Layouts

SelectableDelegate {
    id: root
    height: 42

    required property string itemTitle
    required property string itemSubtitle
    required property string itemType     // "Built-in" | "Raycast Extension" | "Application" | "File" | "Folder"

    property string itemIconSource: ""
    property var itemShortcutTokens: []
    property string itemAlias: ""

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 12
        spacing: 10

        // Icon slot
        Item {
            Layout.preferredWidth: 28
            Layout.preferredHeight: 28
            Layout.alignment: Qt.AlignVCenter

            Image {
                id: appIconImg
                anchors.fill: parent
                visible: root.itemIconSource !== ""
                source: root.itemIconSource
                fillMode: Image.PreserveAspectFit
                mipmap: true
                smooth: true
            }

            Rectangle {
                anchors.fill: parent
                visible: !appIconImg.visible
                radius: 8
                color: root.itemType === "Folder" ? Theme.iconFallbackFolder
                     : root.itemType === "File"   ? Theme.iconFallbackFile
                     : Theme.iconFallbackApp
                border.color: Theme.mainWindowBorder
                border.width: 1
            }
        }

        // Text area — title + subtitle with smart width sharing
        Item {
            id: textRow
            Layout.fillWidth: true
            Layout.minimumWidth: 80
            implicitHeight: titleText.implicitHeight

            readonly property real spacing: 6
            readonly property real shortcutSpace: shortcutBadge.visible ? shortcutBadge.implicitWidth + spacing : 0
            readonly property real aliasSpace: aliasBadge.visible ? aliasBadge.implicitWidth + spacing : 0
            readonly property real reserved: shortcutSpace + aliasSpace
            readonly property real availableForText: width - reserved

            Text {
                id: titleText
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(implicitWidth, textRow.availableForText - (subtitleText.visible ? textRow.spacing + subtitleReserved : 0))
                text: root.itemTitle
                color: root.selected ? Theme.listItemSelectionFg : (root.hovered ? Theme.listItemHoverFg : Theme.foreground)
                font.family: Theme.fontFamily
                font.pixelSize: Theme.regularFontSize
                font.weight: Font.Medium
                elide: Text.ElideRight
                maximumLineCount: 1

                readonly property real subtitleReserved: subtitleText.visible ? Math.min(subtitleText.implicitWidth + textRow.spacing, textRow.availableForText * 0.45) : 0
            }

            Text {
                id: subtitleText
                visible: root.itemSubtitle !== ""
                anchors.left: titleText.right
                anchors.leftMargin: visible ? textRow.spacing : 0
                anchors.baseline: titleText.baseline
                width: Math.min(implicitWidth, Math.max(0, textRow.availableForText - titleText.width - textRow.spacing))
                text: root.itemSubtitle
                color: root.selected ? Theme.listItemSecondarySelectionFg : (root.hovered ? Theme.listItemSecondaryHoverFg : Theme.textMuted)
                font.family: Theme.fontFamily
                font.pixelSize: Theme.smallerFontSize
                elide: Text.ElideRight
                maximumLineCount: 1
            }

            TextBadge {
                id: aliasBadge
                visible: root.itemAlias !== ""
                anchors.left: subtitleText.visible ? subtitleText.right : titleText.right
                anchors.leftMargin: visible ? textRow.spacing : 0
                anchors.verticalCenter: parent.verticalCenter
                text: root.itemAlias
            }

            ShortcutBadge {
                id: shortcutBadge
                visible: root.itemShortcutTokens.length > 0
                anchors.left: aliasBadge.visible ? aliasBadge.right : (subtitleText.visible ? subtitleText.right : titleText.right)
                anchors.leftMargin: visible ? textRow.spacing + 4 : 0
                anchors.verticalCenter: parent.verticalCenter
                tokens: root.itemShortcutTokens
                contentColor: root.selected ? Theme.listItemSelectionFg : Theme.textMuted
            }
        }

        // Right-side category badge
        TextBadge {
            visible: root.itemType !== ""
            text: root.itemType
            contentColor: Theme.categoryColor(root.itemType)
            Layout.alignment: Qt.AlignVCenter
        }
    }
}
