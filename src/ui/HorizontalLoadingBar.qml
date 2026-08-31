import QtQuick

Item {
    id: root
    property bool loading: false
    clip: true

    property bool _active: false

    Rectangle {
        anchors.fill: parent
        color: Theme.mainWindowBorder
    }

    Rectangle {
        id: bar
        width: 60
        height: parent.height
        y: 0
        visible: root._active
        color: Theme.accent
        opacity: 0.7
    }

    NumberAnimation {
        id: slideAnimation
        target: bar
        property: "x"
        from: -bar.width
        to: root.width
        duration: Math.max(600, (root.width + bar.width) / 10 * 10)
        loops: Animation.Infinite
        running: root._active
    }

    Timer {
        id: debounce
        interval: 120
        onTriggered: {
            if (root.loading) {
                bar.x = -bar.width;
                root._active = true;
            } else {
                root._active = false;
            }
        }
    }

    onLoadingChanged: debounce.restart()
}
