import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: rootWindow
    width:  Config.windowWidth
    height: Config.windowHeight
    visible: true
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"

    onVisibleChanged: {
        if (visible) {
            searchInput.forceActiveFocus();
            searchInput.selectAll();
        }
    }

    // Center on screen — matching Vicinae Component.onCompleted positioning
    Component.onCompleted: {
        rootWindow.x = Screen.virtualX + (Screen.width  - rootWindow.width)  / 2;
        rootWindow.y = Screen.virtualY + (Screen.height - rootWindow.height) / 3;
        searchInput.forceActiveFocus();
    }

    // Main launcher card shell — styled with Fluent UI Acrylic / Mica semi-transparency
    Rectangle {
        id: mainCard
        anchors.fill: parent
        radius: Config.borderRounding
        color: Qt.rgba(Theme.background.r, Theme.background.g, Theme.background.b, 0.78)
        border.color: Theme.mainWindowBorder
        border.width: Config.borderWidth

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Config.borderWidth
            spacing: 0

            // Search Bar ──────────
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 52

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 12

                    // Magnifier icon
                    Text {
                        text: "⌕"
                        font.pixelSize: 20
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        Layout.alignment: Qt.AlignVCenter
                    }

                    // Search input
                    TextInput {
                        id: searchInput
                        Layout.fillWidth: true
                        verticalAlignment: TextInput.AlignVCenter
                        color: Theme.foreground
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.regularFontSize * 1.2
                        font.weight: Font.Medium
                        clip: true
                        focus: true

                        // Placeholder
                        Text {
                            anchors.fill: parent
                            verticalAlignment: Text.AlignVCenter
                            text: "Search apps, files, commands…"
                            color: Theme.textPlaceholder
                            font: searchInput.font
                            visible: searchInput.text === "" && !searchInput.activeFocus
                        }

                        onTextChanged: searchModel.query = searchInput.text
                        KeyNavigation.down: resultsList
                    }

                    ShortcutBadge {
                        tokens: [{ text: "Alt" }, { text: "␣" }]
                        contentColor: Theme.textMuted
                        Layout.alignment: Qt.AlignVCenter
                    }
                }
            }

            // Loading Bar / Divider 
            HorizontalLoadingBar {
                Layout.fillWidth: true
                Layout.preferredHeight: 2
                loading: searchModel.isLoading
            }

            // Results List 
            ResultsListView {
                id: resultsList
                Layout.fillWidth: true
                Layout.fillHeight: true

                onItemActivated: function(index) {
                    console.log("[Atlas] Activated index:", index);
                }
            }

            // Divider 
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: Theme.mainWindowBorder
            }

            // Footer 
            Footer {
                id: footer
                Layout.fillWidth: true
                navigationTitle: searchModel.selectedTitle
                actionPanelOpen: actionPanel.open

                onPrimaryActionRequested: {
                    searchModel.activateSelected();
                }
                onActionsToggleRequested: {
                    actionPanel.open = !actionPanel.open;
                }
            }
        }

        // Action Panel Popover 
        // Anchored bottom-right of the card, above the footer (40px)
        ActionPanelPopover {
            id: actionPanel
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 42   // footer height + 2px gap

            onActionTriggered: function(index, title) {
                console.log("[Atlas] Action triggered:", title);
            }
        }
    }

    // Keyboard shortcuts 

    Shortcut {
        sequence: "Down"
        onActivated: {
            if (actionPanel.open) { actionPanel.moveDown(); return; }
            resultsList.moveDown();
        }
    }

    Shortcut {
        sequence: "Up"
        onActivated: {
            if (actionPanel.open) { actionPanel.moveUp(); return; }
            resultsList.moveUp();
        }
    }

    Shortcut {
        sequence: "Return"
        onActivated: {
            if (actionPanel.open) { actionPanel.activateCurrent(); return; }
            resultsList.activateCurrent();
        }
    }

    Shortcut {
        sequence: "Ctrl+K"
        onActivated: actionPanel.open = !actionPanel.open
    }

    Shortcut {
        sequence: "Escape"
        onActivated: {
            if (actionPanel.open) {
                actionPanel.open = false;
                return;
            }
            rootWindow.close();
        }
    }
}
