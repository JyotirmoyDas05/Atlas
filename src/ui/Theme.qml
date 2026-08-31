pragma Singleton
import QtQuick

QtObject {
    // Vicinae Dark Palette Tokens
    readonly property color background: "#141416"
    readonly property color secondaryBackground: "#18181b"
    readonly property color mainWindowBorder: "#27272a"
    readonly property color foreground: "#f4f4f5"
    readonly property color textMuted: "#a1a1aa"
    readonly property color textPlaceholder: "#71717a"
    
    readonly property color inputBackground: "#1c1c20"
    readonly property color inputBorder: "#27272a"
    readonly property color inputBorderFocus: "#3f3f46"

    readonly property color listItemSelectionBg: "#2d2d32"
    readonly property color listItemSelectionFg: "#ffffff"
    readonly property color listItemHoverBg: "#202024"
    readonly property color listItemHoverFg: "#f4f4f5"
    readonly property color listItemSecondarySelectionFg: "#d4d4d8"
    readonly property color listItemSecondaryHoverFg: "#a1a1aa"

    readonly property color badgeBackground: "#27272a"
    readonly property color badgeText: "#d4d4d8"
    readonly property color badgeBorder: "#3f3f46"

    readonly property color accent: "#FF6363"
    readonly property color buttonPrimaryBg: "#FF6363"
    readonly property color buttonPrimaryHoverBg: "#ff7575"
    readonly property color buttonSecondaryBg: "#27272a"

    readonly property int regularFontSize: 13
    readonly property int smallerFontSize: 11
    readonly property int titleFontSize: 15

    readonly property string fontFamily: "Segoe UI, Inter, sans-serif"
}
