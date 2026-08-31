#include "ClipboardHistoryViewHost.hpp"

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

ClipboardHistoryViewHost::ClipboardHistoryViewHost(ClipboardService *clipboard, QObject *parent)
    : ViewHost(parent), m_clipboard(clipboard), m_section(clipboard) {
    m_model.addSource(&m_section);
    m_model.setFilterText({});

    connect(m_clipboard, &ClipboardService::historyChanged, this,
            [this] { m_model.setFilterText(m_filter); });
}
