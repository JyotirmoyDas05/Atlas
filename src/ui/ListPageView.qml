import QtQuick

// Generic pushed list page: renders a ViewHost that exposes a `model`
// property (a SectionListModel). The palette window duck-types against
// moveUp/moveDown/activateCurrent for keyboard routing.
Item {
    id: page

    required property var viewHost

    function moveDown()       { list.moveDown(); }
    function moveUp()         { list.moveUp(); }
    function activateCurrent(){ list.activateCurrent(); }

    ResultsListView {
        id: list
        anchors.fill: parent
        listModel: page.viewHost.model
        emptyTitle: "Nothing here yet"
        emptyDescription: "No entries match your search"
    }
}
