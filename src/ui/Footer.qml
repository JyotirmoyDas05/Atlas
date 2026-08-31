import QtQuick
import QtQuick.Layouts

Item {
    id: root

    // Bound to the selected item title from the search model
    property string navigationTitle: ""
    property bool actionPanelOpen: false
    property string primaryLabel: "Open"
    property var primaryShortcutTokens: [{ text: "↵" }]

    signal primaryActionRequested
    signal actionsToggleRequested

    implicitHeight: 40

    // Elevated bar so the footer reads as a status bar, not floating text
    // over the same background as the list content.
    Rectangle {
        anchors.fill: parent
        color: Theme.elevatedBackground
    }

    // Top border separating content from footer
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: Theme.mainWindowBorder
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 4

        // Left nav status — shows selected item title
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Row {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                spacing: 6

                Text {
                    text: root.navigationTitle !== "" ? root.navigationTitle : "Atlas"
                    color: root.navigationTitle !== "" ? Theme.textMuted : Theme.accent
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.smallerFontSize
                    font.weight: Font.Medium
                    elide: Text.ElideRight
                    width: Math.min(implicitWidth, 280)
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        // Primary action button (Open / ↵)
        FooterButton {
            id: primaryButton
            label: root.primaryLabel
            shortcutTokens: root.primaryShortcutTokens
            highlighted: true
            Layout.alignment: Qt.AlignVCenter
            onClicked: root.primaryActionRequested()
        }

        // Separator
        Rectangle {
            Layout.alignment: Qt.AlignVCenter
            width: 1
            height: 14
            color: Theme.mainWindowBorder
            opacity: primaryButton.hovered || actionsButton.hovered ? 0.0 : 0.5

            Behavior on opacity { NumberAnimation { duration: 150 } }
        }

        // Actions toggle button (Ctrl+K)
        FooterButton {
            id: actionsButton
            label: "Actions"
            shortcutTokens: [{ "text": "Ctrl" }, { "text": "K" }]
            highlighted: root.actionPanelOpen
            backgrounded: root.actionPanelOpen
            Layout.alignment: Qt.AlignVCenter
            onClicked: root.actionsToggleRequested()
        }
    }
}
