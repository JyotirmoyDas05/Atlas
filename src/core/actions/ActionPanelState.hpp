#pragma once

// Adapted from Vicinae (https://github.com/vicinaehq/vicinae),
// src/server/src/ui/action-pannel/action-panel-state.hpp.
// Copyright (C) Vicinae contributors. Licensed under the GNU General
// Public License v3.0 (see LICENSE).

#include "Action.hpp"

#include <QString>
#include <memory>
#include <numeric>
#include <vector>

struct ActionPanelSection {
    QString name;
    std::vector<std::shared_ptr<Action>> actions;
};

class ActionPanelState {
public:
    enum class ShortcutPreset { None, List, Form };

    explicit ActionPanelState(ShortcutPreset preset = ShortcutPreset::List) {
        setShortcutPreset(preset);
    }

    ActionPanelSection *createSection(const QString &name = {}) {
        m_sections.push_back(std::make_unique<ActionPanelSection>(ActionPanelSection{.name = name}));
        return m_sections.back().get();
    }

    const std::vector<std::unique_ptr<ActionPanelSection>> &sections() const { return m_sections; }

    int actionCount() const {
        return std::accumulate(m_sections.begin(), m_sections.end(), 0,
                               [](int acc, auto &s) { return acc + static_cast<int>(s->actions.size()); });
    }

    Action *primaryAction() const { return m_primary; }

    void setShortcutPreset(ShortcutPreset preset) {
        switch (preset) {
        case ShortcutPreset::List:
            m_defaultShortcuts = {ActionShortcut::enter(), ActionShortcut::shiftEnter()};
            break;
        case ShortcutPreset::Form:
            m_defaultShortcuts = {ActionShortcut::ctrlEnter()};
            break;
        case ShortcutPreset::None:
            m_defaultShortcuts.clear();
            break;
        }
    }

    // Compute the primary action (first action, unless one is explicitly
    // marked) and assign the preset's shortcuts to the first section's
    // leading actions. Call once the panel's actions are fully built.
    void finalize() {
        ActionPanelSection *primarySection = computePrimaryAction();
        applyShortcuts(primarySection);
    }

    // Find the action bound to this key combination, if any.
    Action *findByShortcut(Qt::Key key, Qt::KeyboardModifiers modifiers) const {
        for (auto &section : m_sections)
            for (auto &action : section->actions)
                if (action->isBoundTo(key, modifiers))
                    return action.get();
        return nullptr;
    }

private:
    ActionPanelSection *computePrimaryAction() {
        Action *first = nullptr;
        for (auto &section : m_sections) {
            for (auto &action : section->actions) {
                if (!first)
                    first = action.get();
                if (action->isPrimary()) {
                    m_primary = action.get();
                    return section.get();
                }
            }
        }
        if (first) {
            first->setPrimary(true);
            m_primary = first;
        }
        return m_sections.empty() ? nullptr : m_sections.front().get();
    }

    void applyShortcuts(ActionPanelSection *primarySection) {
        if (!primarySection)
            return;
        for (std::size_t i = 0; i < primarySection->actions.size() && i < m_defaultShortcuts.size(); ++i) {
            auto &action = primarySection->actions[i];
            const auto existing = action->shortcut();
            action->setShortcut(m_defaultShortcuts[i]);
            if (existing)
                action->addShortcut(*existing);
        }
    }

    std::vector<std::unique_ptr<ActionPanelSection>> m_sections;
    std::vector<ActionShortcut> m_defaultShortcuts;
    Action *m_primary = nullptr;
};
