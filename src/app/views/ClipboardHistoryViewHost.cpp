#include "ClipboardHistoryViewHost.hpp"

#include "core/actions/ActionPanelState.hpp"
#include "services/clipboard/ClipboardService.hpp"

QVariantMap ClipboardHistoryViewHost::HistorySection::item(int index) const {
    QVariantMap m = m_items.value(index).toMap();
    m["itemId"] = "clip:" + m.value("id").toString();
    if (m.value("pinned").toBool())
        m["alias"] = QStringLiteral("pinned");
    return m;
}

void ClipboardHistoryViewHost::HistorySection::setFilter(const QString &query) {
    m_items = m_clipboard->history(query, 100);
}

void ClipboardHistoryViewHost::HistorySection::activate(int index) {
    m_clipboard->copyEntry(m_items.value(index).toMap().value("id").toString());
}

std::unique_ptr<ActionPanelState>
ClipboardHistoryViewHost::HistorySection::actionsFor(int index) const {
    const QVariantMap entry = m_items.value(index).toMap();
    const QString id = entry.value("id").toString();
    const bool pinned = entry.value("pinned").toBool();
    ClipboardService *clipboard = m_clipboard;

    auto state = std::make_unique<ActionPanelState>(ActionPanelState::ShortcutPreset::List);
    ActionPanelSection *section = state->createSection();

    auto copy = std::make_shared<Action>(QStringLiteral("Copy to Clipboard"),
                                         [clipboard, id] { clipboard->copyEntry(id); });
    copy->setPrimary(true);
    section->actions.push_back(copy);

    section->actions.push_back(std::make_shared<Action>(
        pinned ? QStringLiteral("Unpin") : QStringLiteral("Pin"),
        [clipboard, id, pinned] { clipboard->pinEntry(id, !pinned); }));

    auto remove = std::make_shared<Action>(QStringLiteral("Delete"),
                                           [clipboard, id] { clipboard->removeEntry(id); });
    remove->setStyle(Action::Style::Danger);
    section->actions.push_back(remove);

    state->finalize();
    return state;
}

ClipboardHistoryViewHost::ClipboardHistoryViewHost(ClipboardService *clipboard, QObject *parent)
    : ListViewHostBase(parent), m_clipboard(clipboard), m_section(clipboard) {
    m_model.addSource(&m_section);
    m_model.setFilterText({});

    connect(m_clipboard, &ClipboardService::historyChanged, this,
            [this] { m_model.setFilterText(m_filter); });
}
