#pragma once

#include <QObject>
#include <QColor>
#include <QString>

class ThemeBridge : public QObject {
    Q_OBJECT

    Q_PROPERTY(QColor background READ background CONSTANT)
    Q_PROPERTY(QColor secondaryBackground READ secondaryBackground CONSTANT)
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
    Q_PROPERTY(QColor badgeBackground READ badgeBackground CONSTANT)
    Q_PROPERTY(QColor badgeBorder READ badgeBorder CONSTANT)
    Q_PROPERTY(QColor badgeText READ badgeText CONSTANT)
    Q_PROPERTY(QColor buttonPrimaryBg READ buttonPrimaryBg CONSTANT)
    Q_PROPERTY(QColor buttonPrimaryHoverBg READ buttonPrimaryHoverBg CONSTANT)
    Q_PROPERTY(QColor buttonSecondaryBg READ buttonSecondaryBg CONSTANT)
    Q_PROPERTY(QColor buttonSecondaryHoverBg READ buttonSecondaryHoverBg CONSTANT)
    Q_PROPERTY(int regularFontSize READ regularFontSize CONSTANT)
    Q_PROPERTY(int smallerFontSize READ smallerFontSize CONSTANT)
    Q_PROPERTY(QString fontFamily READ fontFamily CONSTANT)

public:
    explicit ThemeBridge(QObject *parent = nullptr) : QObject(parent) {}

    // Exact Vicinae Dark Palette Tokens
    QColor background() const { return QColor("#141416"); }
    QColor secondaryBackground() const { return QColor("#18181b"); }
    QColor foreground() const { return QColor("#f4f4f5"); }
    QColor textMuted() const { return QColor("#a1a1aa"); }
    QColor textPlaceholder() const { return QColor("#71717a"); }
    QColor mainWindowBorder() const { return QColor("#27272a"); }
    QColor inputBackground() const { return QColor("#1c1c20"); }
    QColor inputBorder() const { return QColor("#27272a"); }
    QColor inputBorderFocus() const { return QColor("#3f3f46"); }

    QColor listItemSelectionBg() const { return QColor("#2d2d32"); }
    QColor listItemSelectionFg() const { return QColor("#ffffff"); }
    QColor listItemHoverBg() const { return QColor("#202024"); }
    QColor listItemHoverFg() const { return QColor("#f4f4f5"); }
    QColor listItemSecondarySelectionFg() const { return QColor("#d4d4d8"); }
    QColor listItemSecondaryHoverFg() const { return QColor("#a1a1aa"); }

    QColor badgeBackground() const { return QColor("#27272a"); }
    QColor badgeBorder() const { return QColor("#3f3f46"); }
    QColor badgeText() const { return QColor("#d4d4d8"); }

    QColor accent() const { return QColor("#FF6363"); }
    QColor buttonPrimaryBg() const { return QColor("#FF6363"); }
    QColor buttonPrimaryHoverBg() const { return QColor("#ff7575"); }
    QColor buttonSecondaryBg() const { return QColor("#27272a"); }
    QColor buttonSecondaryHoverBg() const { return QColor("#323238"); }

    int regularFontSize() const { return 13; }
    int smallerFontSize() const { return 11; }
    QString fontFamily() const { return QStringLiteral("Segoe UI, Inter, sans-serif"); }
};
