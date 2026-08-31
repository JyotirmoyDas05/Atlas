import QtQuick
import QtQuick.Controls

// Results list with keyboard navigation, calculator support, empty view, and wheel-driven selection change
// Matches Vicinae's GenericListView + RootSearchList pattern
Item {
    id: root

    property alias currentIndex: listView.currentIndex
    property alias count: listView.count

    signal itemActivated(int index)
    signal itemSelected(int index)

    function moveDown() {
        const next = searchModel.nextSelectableIndex(listView.currentIndex, 1);
        if (next !== listView.currentIndex) {
            listView.currentIndex = next;
            listView.positionViewAtIndex(next, ListView.Contain);
        }
    }

    function moveUp() {
        const next = searchModel.nextSelectableIndex(listView.currentIndex, -1);
        if (next !== listView.currentIndex) {
            listView.currentIndex = next;
            listView.positionViewAtIndex(next, ListView.Contain);
        }
    }

    function activateCurrent() {
        root.itemActivated(listView.currentIndex);
        searchModel.activateSelected();
    }

    // Empty View
    EmptyView {
        anchors.centerIn: parent
        visible: listView.count === 0 && !searchModel.isLoading
        title: searchModel.query === "" ? "No recent items" : "No results found"
        description: searchModel.query === "" ? "Type to search apps, files, snippets, and math expressions" : "No matches for '" + searchModel.query + "'"
    }

    ListView {
        id: listView
        anchors.fill: parent
        clip: true
        reuseItems: true
        cacheBuffer: 200

        // Vicinae: interactive: false — no flick, only programmatic scroll
        interactive: false
        boundsBehavior: Flickable.StopAtBounds
        highlightMoveDuration: 0
        topMargin: 4
        bottomMargin: 4

        model: searchModel
        currentIndex: searchModel.selectedIndex

        onCurrentIndexChanged: {
            searchModel.selectedIndex = currentIndex;
            root.itemSelected(currentIndex);
        }

        // Sync when the model rebuilds (new query / async results landing)
        Connections {
            target: searchModel
            function onSelectedIndexChanged() {
                listView.currentIndex = searchModel.selectedIndex;
                if (listView.currentIndex >= 0)
                    listView.positionViewAtIndex(listView.currentIndex, ListView.Contain);
            }
        }

        ScrollBar.vertical: ViciScrollBar {
            policy: listView.contentHeight > listView.height ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
        }

        // Wheel handler — maps scroll to selection change (Vicinae ViciWheelHandler pattern)
        WheelHandler {
            onWheel: function(event) {
                if (event.angleDelta.y < 0)
                    root.moveDown();
                else
                    root.moveUp();
                event.accepted = true;
            }
        }

        delegate: Loader {
            id: delegateLoader
            width: listView.width

            required property int index
            required property var model

            readonly property bool isHeader: delegateLoader.model.isSection === true
            readonly property bool isCalc:   delegateLoader.model.isCalculator === true

            sourceComponent: isHeader ? headerComponent : (isCalc ? calcComponent : itemComponent)

            Component {
                id: headerComponent

                SectionHeader {
                    width: delegateLoader.width
                    text: delegateLoader.model.title || ""
                }
            }

            Component {
                id: calcComponent

                CalculatorResultDelegate {
                    width: delegateLoader.width
                    calcQuestion:     delegateLoader.model.calcQuestion     || ""
                    calcQuestionUnit: delegateLoader.model.calcQuestionUnit || ""
                    calcAnswer:       delegateLoader.model.calcAnswer       || ""
                    calcAnswerUnit:   delegateLoader.model.calcAnswerUnit   || ""

                    selected: listView.currentIndex === delegateLoader.index

                    onClicked: {
                        listView.currentIndex = delegateLoader.index;
                    }
                    onActivated: {
                        listView.currentIndex = delegateLoader.index;
                        root.itemActivated(delegateLoader.index);
                        searchModel.activateRow(delegateLoader.index);
                    }
                }
            }

            Component {
                id: itemComponent

                ListItemDelegate {
                    width: delegateLoader.width
                    itemTitle:      delegateLoader.model.title    || ""
                    itemSubtitle:   delegateLoader.model.subtitle || ""
                    itemType:       delegateLoader.model.type     || ""
                    itemAlias:      delegateLoader.model.alias    || ""
                    itemIconSource: delegateLoader.model.icon     || ""

                    selected: listView.currentIndex === delegateLoader.index

                    onClicked: {
                        listView.currentIndex = delegateLoader.index;
                    }
                    onActivated: {
                        listView.currentIndex = delegateLoader.index;
                        root.itemActivated(delegateLoader.index);
                        searchModel.activateRow(delegateLoader.index);
                    }
                }
            }
        }
    }
}
