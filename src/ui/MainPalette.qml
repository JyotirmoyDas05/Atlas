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

                    // Search input — routes to root search or the pushed view
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
                            text: nav.searchPlaceholder
                            color: Theme.textPlaceholder
                            font: searchInput.font
                            visible: searchInput.text === "" && !searchInput.activeFocus
                        }

                        onTextChanged: {
                            if (nav.depth > 0)
                                nav.currentView.setFilterText(searchInput.text);
                            else
                                searchModel.query = searchInput.text;
                        }

                        Keys.onPressed: function(event) {
                            if (event.key === Qt.Key_Backspace && searchInput.text === "" && nav.depth > 0) {
                                nav.pop();
                                event.accepted = true;
                            }
                        }
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

            // View stack: root search at the bottom, pushed views above it
            StackView {
                id: viewStack
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                initialItem: rootResultsComponent

                // Palette navigation is instant, never animated.
                pushEnter: null
                pushExit: null
                popEnter: null
                popExit: null
                replaceEnter: null
                replaceExit: null
            }

            Component {
                id: rootResultsComponent

                ResultsListView {
                    listModel: searchModel
                    loading: searchModel.isLoading
                    emptyTitle: searchModel.query === "" ? "No recent items" : "No results found"
                    emptyDescription: searchModel.query === ""
                        ? "Type to search apps, files, snippets, and math expressions"
                        : "No matches for '" + searchModel.query + "'"
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
                navigationTitle: nav.depth > 0 ? nav.currentTitle : searchModel.selectedTitle
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

    // Navigation stack mirror — C++ Navigation drives the QML StackView
    Connections {
        target: nav

        function onViewPushed(componentUrl, properties) {
            viewStack.push(componentUrl, properties, StackView.Immediate);
            searchInput.clear();
            searchInput.forceActiveFocus();
        }

        function onViewPopped() {
            viewStack.pop(StackView.Immediate);
            searchInput.clear();
            searchInput.forceActiveFocus();
        }
    }

    // Keyboard shortcuts — routed to the current stack item (duck-typed
    // moveUp/moveDown/activateCurrent contract)

    Shortcut {
        sequence: "Down"
        onActivated: {
            if (actionPanel.open) { actionPanel.moveDown(); return; }
            viewStack.currentItem.moveDown();
        }
    }

    Shortcut {
        sequence: "Up"
        onActivated: {
            if (actionPanel.open) { actionPanel.moveUp(); return; }
            viewStack.currentItem.moveUp();
        }
    }

    Shortcut {
        sequence: "Return"
        onActivated: {
            if (actionPanel.open) { actionPanel.activateCurrent(); return; }
            viewStack.currentItem.activateCurrent();
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
            if (nav.depth > 0) {
                nav.pop();
                return;
            }
            rootWindow.close();
        }
    }
}
