import QtQuick
import QtQuick.Controls.Basic   // Force Basic style for customization

ScrollBar {
    id: control

    property bool _recentlyScrolled: false

    function revealOnScroll() {
        _recentlyScrolled = true;
        scrollActivityTimer.restart();
    }

    onPositionChanged: revealOnScroll()

    Timer {
        id: scrollActivityTimer
        interval: 400
        onTriggered: control._recentlyScrolled = false
    }

    contentItem: Rectangle {
        implicitWidth: 5
        implicitHeight: 5
        radius: 3
        color: Theme.textMuted
        opacity: control.active || control._recentlyScrolled ? 0.45 : 0.0

        Behavior on opacity {
            NumberAnimation { duration: 200 }
        }
    }

    background: Item {}
}
