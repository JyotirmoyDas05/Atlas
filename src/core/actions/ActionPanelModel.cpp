#include "ActionPanelModel.hpp"

#include <algorithm>

ActionPanelModel::ActionPanelModel(QObject *parent) : QAbstractListModel(parent) {}

void ActionPanelModel::setState(std::unique_ptr<ActionPanelState> state) {
    m_state = std::move(state);
    rebuild();
}

void ActionPanelModel::rebuild() {
    beginResetModel();
    m_rows.clear();
    if (m_state) {
        const auto &sections = m_state->sections();
        for (int s = 0; s < static_cast<int>(sections.size()); ++s) {
            if (sections[s]->actions.empty())
                continue;
            if (!sections[s]->name.isEmpty())
                m_rows.push_back({.isHeader = true, .sectionIndex = s});
            for (int a = 0; a < static_cast<int>(sections[s]->actions.size()); ++a)
                m_rows.push_back({.isHeader = false, .sectionIndex = s, .actionIndex = a});
        }
    }
    endResetModel();
    emit countChanged();
    setSelectedIndex(nextSelectableIndex(-1, 1));
}

Action *ActionPanelModel::actionAt(int row) const {
    if (!m_state || row < 0 || row >= static_cast<int>(m_rows.size()))
        return nullptr;
    const FlatRow &r = m_rows[row];
    if (r.isHeader)
        return nullptr;
    return m_state->sections()[r.sectionIndex]->actions[r.actionIndex].get();
}

QVariant ActionPanelModel::data(const QModelIndex &index, int role) const {
    const int i = index.row();
    if (i < 0 || i >= static_cast<int>(m_rows.size()))
        return {};
    const FlatRow &row = m_rows[i];

    if (role == IsSectionRole)
        return row.isHeader;
    if (role == SectionNameRole)
        return m_state->sections()[row.sectionIndex]->name;
    if (row.isHeader)
        return role == TitleRole ? QVariant(m_state->sections()[row.sectionIndex]->name) : QVariant();

    const Action *action = actionAt(i);
    switch (role) {
    case TitleRole:          return action->title();
    case IconRole:           return action->icon();
    case ShortcutTokensRole: {
        const auto sc = action->shortcut();
        return sc ? sc->tokens() : QVariantList{};
    }
    case IsDangerRole:       return action->style() == Action::Style::Danger;
    default:                 return {};
    }
}

QHash<int, QByteArray> ActionPanelModel::roleNames() const {
    return {
        {IsSectionRole, "isSection"},
        {SectionNameRole, "sectionName"},
        {TitleRole, "title"},
        {IconRole, "icon"},
        {ShortcutTokensRole, "shortcutTokens"},
        {IsDangerRole, "isDanger"},
    };
}

void ActionPanelModel::setSelectedIndex(int index) {
    const int count = static_cast<int>(m_rows.size());
    index = count == 0 ? -1 : std::clamp(index, -1, count - 1);
    if (index == m_selectedIndex)
        return;
    m_selectedIndex = index;
    emit selectedIndexChanged();
}

int ActionPanelModel::nextSelectableIndex(int from, int direction) const {
    const int count = static_cast<int>(m_rows.size());
    if (count == 0)
        return -1;
    int next = from + direction;
    while (next >= 0 && next < count) {
        if (!m_rows[next].isHeader)
            return next;
        next += direction;
    }
    return from;
}

void ActionPanelModel::activateSelected() {
    activateRow(m_selectedIndex);
}

void ActionPanelModel::activateRow(int row) {
    if (Action *action = actionAt(row)) {
        action->execute();
        emit actionExecuted();
    }
}

bool ActionPanelModel::tryShortcut(int key, int modifiers) {
    if (!m_state)
        return false;
    Action *action = m_state->findByShortcut(static_cast<Qt::Key>(key),
                                             static_cast<Qt::KeyboardModifiers>(modifiers));
    if (!action)
        return false;
    action->execute();
    emit actionExecuted();
    return true;
}
