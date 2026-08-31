#pragma once

#include <QObject>
#include <QColor>
#include <QHash>
#include <QString>

// Atlas's semantic color/type tokens exposed to QML as the `Theme` context
// property. One dark palette for now (see MEMORY: light theme + user themes
// are future work); the point of routing everything through named tokens
// instead of literal hex in QML is that it stays a one-file change either way.
class ThemeBridge : public QObject {
    Q_OBJECT

    Q_PROPERTY(QColor background READ background CONSTANT)
    Q_PROPERTY(QColor secondaryBackground READ secondaryBackground CONSTANT)
    Q_PROPERTY(QColor elevatedBackground READ elevatedBackground CONSTANT)
    Q_PROPERTY(QColor foreground READ foreground CONSTANT)
    Q_PROPERTY(QColor textMuted READ textMuted CONSTANT)
    Q_PROPERTY(QColor textPlaceholder READ textPlaceholder CONSTANT)
    Q_PROPERTY(QColor mainWindowBorder READ mainWindowBorder CONSTANT)
    Q_PROPERTY(QColor inputBackground READ inputBackground CONSTANT)
    Q_PROPERTY(QColor inputBorder READ inputBorder CONSTANT)
    Q_PROPERTY(QColor inputBorderFocus READ inputBorderFocus CONSTANT)
    Q_PROPERTY(QColor listItemSelectionBg READ listItemSelectionBg CONSTANT)
    Q_PROPERTY(QColor listItemSelectionFg READ listItemSelectionFg CONSTANT)
    Q_PROPERTY(QColor listItemHoverBg READ listItemHoverBg CONSTANT)
    Q_PROPERTY(QColor listItemHoverFg READ listItemHoverFg CONSTANT)
    Q_PROPERTY(QColor listItemSecondarySelectionFg READ listItemSecondarySelectionFg CONSTANT)
    Q_PROPERTY(QColor listItemSecondaryHoverFg READ listItemSecondaryHoverFg CONSTANT)
    Q_PROPERTY(QColor accent READ accent CONSTANT)
    Q_PROPERTY(QColor accentForeground READ accentForeground CONSTANT)
    Q_PROPERTY(QColor danger READ danger CONSTANT)
    Q_PROPERTY(QColor success READ success CONSTANT)
    Q_PROPERTY(QColor badgeBackground READ badgeBackground CONSTANT)
    Q_PROPERTY(QColor badgeBorder READ badgeBorder CONSTANT)
    Q_PROPERTY(QColor badgeText READ badgeText CONSTANT)
    Q_PROPERTY(QColor buttonPrimaryBg READ buttonPrimaryBg CONSTANT)
    Q_PROPERTY(QColor buttonPrimaryHoverBg READ buttonPrimaryHoverBg CONSTANT)
    Q_PROPERTY(QColor buttonSecondaryBg READ buttonSecondaryBg CONSTANT)
    Q_PROPERTY(QColor buttonSecondaryHoverBg READ buttonSecondaryHoverBg CONSTANT)
    Q_PROPERTY(QColor iconFallbackApp READ iconFallbackApp CONSTANT)
    Q_PROPERTY(QColor iconFallbackFile READ iconFallbackFile CONSTANT)
    Q_PROPERTY(QColor iconFallbackFolder READ iconFallbackFolder CONSTANT)
    Q_PROPERTY(int regularFontSize READ regularFontSize CONSTANT)
    Q_PROPERTY(int smallerFontSize READ smallerFontSize CONSTANT)
    Q_PROPERTY(int titleFontSize READ titleFontSize CONSTANT)
    Q_PROPERTY(QString fontFamily READ fontFamily CONSTANT)

public:
    explicit ThemeBridge(QObject *parent = nullptr) : QObject(parent) {}

    QColor background() const { return QColor("#131316"); }
    QColor secondaryBackground() const { return QColor("#19191d"); }
    QColor elevatedBackground() const { return QColor("#1d1d22"); }
    QColor foreground() const { return QColor("#f2f2f3"); }
    QColor textMuted() const { return QColor("#96969e"); }
    QColor textPlaceholder() const { return QColor("#6b6b74"); }
    QColor mainWindowBorder() const { return QColor("#28282e"); }
    QColor inputBackground() const { return QColor("#1a1a1f"); }
    QColor inputBorder() const { return QColor("#28282e"); }
    QColor inputBorderFocus() const { return QColor("#3d3d45"); }

    QColor listItemSelectionBg() const { return QColor("#242429"); }
    QColor listItemSelectionFg() const { return QColor("#ffffff"); }
    QColor listItemHoverBg() const { return QColor("#1c1c21"); }
    QColor listItemHoverFg() const { return QColor("#f2f2f3"); }
    QColor listItemSecondarySelectionFg() const { return QColor("#d8d8dc"); }
    QColor listItemSecondaryHoverFg() const { return QColor("#96969e"); }

    QColor badgeBackground() const { return QColor("#26262c"); }
    QColor badgeBorder() const { return QColor("#38383f"); }
    QColor badgeText() const { return QColor("#c7c7cd"); }

    // Raycast-recognizable coral-red; kept distinct from `danger` so a
    // destructive action never gets confused with the brand accent.
    QColor accent() const { return QColor("#FF6363"); }
    QColor accentForeground() const { return QColor("#ffffff"); }
    QColor danger() const { return QColor("#e5484d"); }
    QColor success() const { return QColor("#4ade80"); }

    QColor buttonPrimaryBg() const { return accent(); }
    QColor buttonPrimaryHoverBg() const { return QColor("#ff7a7a"); }
    QColor buttonSecondaryBg() const { return QColor("#232328"); }
    QColor buttonSecondaryHoverBg() const { return QColor("#2c2c33"); }

    QColor iconFallbackApp() const { return QColor("#1e1c22"); }
    QColor iconFallbackFile() const { return QColor("#1e293b"); }
    QColor iconFallbackFolder() const { return QColor("#3b341f"); }

    int regularFontSize() const { return 13; }
    int smallerFontSize() const { return 11; }
    int titleFontSize() const { return 15; }

    // Segoe UI Variable is Windows 11's Fluent system font; on Windows 10 (or
    // if unavailable) Qt/Windows silently substitute the classic Segoe UI, so
    // this degrades safely instead of failing.
    QString fontFamily() const { return QStringLiteral("Segoe UI Variable Text"); }

    // Maps a result's category label to a token color. Centralized so QML
    // delegates don't each carry their own copy of this switch.
    Q_INVOKABLE QColor categoryColor(const QString &category) const {
        static const QHash<QString, QColor> table{
            {"Game", QColor("#fb7185")},       {"Developer", QColor("#38bdf8")},
            {"Browser", QColor("#34d399")},    {"Design", QColor("#fbbf24")},
            {"Productivity", QColor("#a78bfa")}, {"System", QColor("#9ca3af")},
            {"Folder", QColor("#eab308")},
        };
        if (category == QLatin1String("Built-in"))
            return accent();
        return table.value(category, QColor("#c084fc"));
    }

    // Common helper: same color, different opacity. Saves every QML file
    // that wants a translucent tint from hand-writing Qt.rgba(c.r,c.g,c.b,a).
    Q_INVOKABLE QColor withAlpha(const QColor &color, qreal alpha) const {
        QColor c = color;
        c.setAlphaF(static_cast<float>(alpha));
        return c;
    }
};
