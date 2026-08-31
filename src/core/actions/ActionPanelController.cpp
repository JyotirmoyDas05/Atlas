#include "ActionPanelController.hpp"

#include "core/list/SectionListModel.hpp"

ActionPanelController::ActionPanelController(QObject *parent) : QObject(parent) {
    connect(&m_model, &ActionPanelModel::countChanged, this, &ActionPanelController::stateChanged);
    connect(&m_model, &ActionPanelModel::actionExecuted, this, [this] { setOpen(false); });
}

void ActionPanelController::setOpen(bool value) {
    if (m_open == value)
        return;
    // Opening with nothing to show would render an empty popover.
    if (value && !hasActions())
        return;
    m_open = value;
    emit openChanged();
}

QString ActionPanelController::primaryActionTitle() const {
    const ActionPanelState *state = m_model.state();
    const Action *primary = state ? state->primaryAction() : nullptr;
    return primary ? primary->title() : QString();
}

QVariantList ActionPanelController::primaryActionShortcutTokens() const {
    const ActionPanelState *state = m_model.state();
    const Action *primary = state ? state->primaryAction() : nullptr;
    if (!primary)
        return {};
    const auto shortcut = primary->shortcut();
    return shortcut ? shortcut->tokens() : QVariantList{};
}

void ActionPanelController::refreshFrom(const SectionListModel *source) {
    m_model.setState(source ? source->actionsForSelected() : nullptr);
    if (m_open && !hasActions())
        setOpen(false);
    emit stateChanged();
}

void ActionPanelController::moveUp() {
    m_model.setSelectedIndex(m_model.nextSelectableIndex(m_model.selectedIndex(), -1));
}

void ActionPanelController::moveDown() {
    m_model.setSelectedIndex(m_model.nextSelectableIndex(m_model.selectedIndex(), 1));
}

void ActionPanelController::activateCurrent() {
    m_model.activateSelected();
}

bool ActionPanelController::tryShortcut(int key, int modifiers) {
    return m_model.tryShortcut(key, modifiers);
}
