#include "Action.hpp"

#include <QKeySequence>
#include <QVariantMap>

QVariantList ActionShortcut::tokens() const {
    QVariantList out;
    if (modifiers & Qt::ControlModifier) out.append(QVariantMap{{"text", "Ctrl"}});
    if (modifiers & Qt::AltModifier) out.append(QVariantMap{{"text", "Alt"}});
    if (modifiers & Qt::ShiftModifier) out.append(QVariantMap{{"text", "Shift"}});

    switch (key) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        out.append(QVariantMap{{"text", "↵"}}); // Return arrow glyph
        break;
    case Qt::Key_Backspace:
        out.append(QVariantMap{{"text", "⌫"}});
        break;
    default:
        out.append(QVariantMap{{"text", QKeySequence(key).toString()}});
        break;
    }
    return out;
}
