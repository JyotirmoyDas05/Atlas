#pragma once

// Pushable clipboard-history view: one SectionListModel over ClipboardService.

#include "core/list/SectionSource.hpp"
#include "core/nav/ListViewHostBase.hpp"

#include <QVariantList>

class ClipboardService;

class ClipboardHistoryViewHost : public ListViewHostBase {
    Q_OBJECT

public:
    explicit ClipboardHistoryViewHost(ClipboardService *clipboard, QObject *parent = nullptr);

    QUrl qmlComponentUrl() const override {
        return QUrl(QStringLiteral("qrc:/Atlas/src/ui/ListPageView.qml"));
    }
    QString navigationTitle() const override { return QStringLiteral("Clipboard History"); }
    QString searchPlaceholder() const override { return QStringLiteral("Search clipboard history…"); }
    void setFilter(const QString &text) override {
        m_filter = text;
        m_model.setFilterText(text);
    }

private:
    class HistorySection : public SectionSource {
    public:
        explicit HistorySection(ClipboardService *clipboard) : m_clipboard(clipboard) {}

        QString sectionName() const override { return {}; }
        int count() const override { return static_cast<int>(m_items.size()); }
        QVariantMap item(int index) const override;
        void activate(int index) override;
        std::unique_ptr<ActionPanelState> actionsFor(int index) const override;
        void setFilter(const QString &query) override;

    private:
        ClipboardService *m_clipboard;
        QVariantList m_items;
    };

    ClipboardService *m_clipboard;
    HistorySection m_section;
    QString m_filter;
};
