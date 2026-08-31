#pragma once

// Adapted from Vicinae (https://github.com/vicinaehq/vicinae),
// src/server/src/ui/action-pannel/action.hpp.
// Copyright (C) Vicinae contributors. Licensed under the GNU General
// Public License v3.0 (see LICENSE).
//
// Simplified for Atlas: no ApplicationContext/ImageURL indirection yet,
// actions execute a plain std::function<void()>.

#include <QString>
#include <QVariantList>
#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

// A key combination an action can be bound to, e.g. Ctrl+Enter.
struct ActionShortcut {
    Qt::Key key = Qt::Key_unknown;
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;

    bool operator==(const ActionShortcut &other) const {
        return key == other.key && modifiers == other.modifiers;
    }

    static ActionShortcut enter() { return {.key = Qt::Key_Return, .modifiers = Qt::NoModifier}; }
    static ActionShortcut shiftEnter() { return {.key = Qt::Key_Return, .modifiers = Qt::ShiftModifier}; }
    static ActionShortcut ctrlEnter() { return {.key = Qt::Key_Return, .modifiers = Qt::ControlModifier}; }

    // Display tokens for ShortcutBadge.qml: [{text: "Ctrl"}, {text: "Enter"}].
    QVariantList tokens() const;
};

class Action {
public:
    enum class Style { Normal, Danger };

    Action(QString title, std::function<void()> execute)
        : m_title(std::move(title)), m_execute(std::move(execute)) {}
    virtual ~Action() = default;

    QString title() const { return m_title; }
    QString icon() const { return m_icon; }
    void setIcon(QString icon) { m_icon = std::move(icon); }

    Style style() const { return m_style; }
    void setStyle(Style style) { m_style = style; }

    bool isPrimary() const { return m_primary; }
    void setPrimary(bool value) { m_primary = value; }

    // First shortcut is the one shown in the UI; later ones are silent aliases.
    std::optional<ActionShortcut> shortcut() const {
        return m_shortcuts.empty() ? std::nullopt : std::make_optional(m_shortcuts.front());
    }
    void setShortcut(const ActionShortcut &shortcut) { m_shortcuts = {shortcut}; }
    void addShortcut(const ActionShortcut &shortcut) { m_shortcuts.push_back(shortcut); }
    bool isBoundTo(Qt::Key key, Qt::KeyboardModifiers modifiers) const {
        const ActionShortcut probe{.key = key, .modifiers = modifiers};
        return std::ranges::any_of(m_shortcuts, [&](auto &s) { return s == probe; });
    }

    void execute() const {
        if (m_execute)
            m_execute();
    }

private:
    QString m_title;
    QString m_icon;
    Style m_style = Style::Normal;
    bool m_primary = false;
    std::vector<ActionShortcut> m_shortcuts;
    std::function<void()> m_execute;
};
