import QtQuick
import QtQuick.Controls

// Floating action panel popup, driven by the global `actionPanel` controller
// (Ctrl+K). Up/Down/Return/Escape are handled by the window's Shortcut items
// calling into actionPanel directly.
Rectangle {
    id: root

    visible: actionPanel.open
    width: 380
    height: Math.min(actionList.count * 34 + 16, 280)
    radius: 10
    color: Theme.secondaryBackground
    border.color: Theme.mainWindowBorder
    border.width: 1

    opacity: actionPanel.open ? 1.0 : 0.0
    scale: actionPanel.open ? 1.0 : 0.95
    transformOrigin: Item.BottomRight

    Behavior on opacity { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
    Behavior on scale   { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }

    Column {
        anchors.fill: parent
        anchors.topMargin: 8
        anchors.bottomMargin: 8
        spacing: 0

        Repeater {
            id: actionList
            model: actionPanel.model

            delegate: Loader {
                id: delegateLoader
                width: root.width

                required property int index
                required property var model

                sourceComponent: delegateLoader.model.isSection ? headerComponent : itemComponent

                Component {
                    id: headerComponent

                    SectionHeader {
                        width: delegateLoader.width
                        text: delegateLoader.model.title || ""
                    }
                }

                Component {
                    id: itemComponent

                    ActionItemDelegate {
                        width: delegateLoader.width
                        actionTitle:    delegateLoader.model.title          || ""
                        actionShortcut: delegateLoader.model.shortcutTokens || []
                        isDanger:       delegateLoader.model.isDanger === true
                        selected: actionPanel.model.selectedIndex === delegateLoader.index

                        onActivated: actionPanel.model.activateRow(delegateLoader.index)
                    }
                }
            }
        }
    }
}
