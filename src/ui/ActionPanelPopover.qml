import QtQuick
import QtQuick.Controls

// Floating action panel popup — triggered by Ctrl+K
Rectangle {
    id: root

    property bool open: false
    property string selectedItemTitle: ""

    // Static action list — will be made dynamic when extension model lands
    readonly property var actions: [
        { title: "Open",                   shortcut: [{ text: "↵" }] },
        { title: "Copy to Clipboard",      shortcut: [{ text: "⌘" }, { text: "C" }] },
        { title: "Show in File Explorer",  shortcut: [{ text: "⌘" }, { text: "O" }] },
        { title: "Configure Shortcut",     shortcut: [{ text: "⌘" }, { text: "K" }] },
    ]

    signal actionTriggered(int index, string title)
    signal closed

    visible: root.open
    width: 380
    height: Math.min(actionList.count * 34 + 16, 280)
    radius: 10
    color: Theme.secondaryBackground
    border.color: Theme.mainWindowBorder
    border.width: 1

    // Entry animation
    opacity: root.open ? 1.0 : 0.0
    scale: root.open ? 1.0 : 0.95
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
            model: root.actions

            delegate: ActionItemDelegate {
                required property int index
                required property var modelData

                width: root.width
                actionTitle: modelData.title
                actionShortcut: modelData.shortcut
                selected: index === root._navIndex

                onActivated: {
                    root.actionTriggered(index, modelData.title);
                    root.open = false;
                    root.closed();
                }
            }
        }
    }

    // Navigation index — tracked as a simple property
    property int _navIndex: 0

    function moveUp() {
        if (_navIndex > 0) _navIndex--;
    }
    function moveDown() {
        if (_navIndex < actions.length - 1) _navIndex++;
    }
    function activateCurrent() {
        if (_navIndex >= 0 && _navIndex < actions.length) {
            const a = actions[_navIndex];
            actionTriggered(_navIndex, a.title);
            open = false;
            closed();
        }
    }

    onOpenChanged: {
        if (open) _navIndex = 0;
    }
}
