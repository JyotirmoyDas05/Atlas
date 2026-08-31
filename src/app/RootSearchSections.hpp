#pragma once

// The root palette's section sources: calculator, applications, snippets,
// files. Each owns its filtering/activation; SectionListModel owns the rest.

#include "core/list/SectionSource.hpp"

#include <QFutureWatcher>
#include <QObject>
#include <QVariantList>
#include <functional>

class CalculatorService;
class AppSearchService;
class SnippetService;
class FileIndexService;
class WinFileSearchService;

class CalculatorSection : public SectionSource {
public:
    explicit CalculatorSection(CalculatorService *calculator);

    QString sectionName() const override { return {}; } // no header row
    int count() const override { return static_cast<int>(m_items.size()); }
    QVariantMap item(int index) const override { return m_items.value(index).toMap(); }
    void activate(int index) override;
    void setFilter(const QString &query) override;
    std::unique_ptr<ActionPanelState> actionsFor(int index) const override;

private:
    CalculatorService *m_calculator;
    QVariantList m_items;
};

class AppSection : public SectionSource {
public:
    explicit AppSection(AppSearchService *apps);

    QString sectionName() const override { return QStringLiteral("Applications"); }
    int count() const override { return static_cast<int>(m_items.size()); }
    QVariantMap item(int index) const override { return m_items.value(index).toMap(); }
    void activate(int index) override;
    void setFilter(const QString &query) override;
    std::unique_ptr<ActionPanelState> actionsFor(int index) const override;

    // Re-pull the app list from the service (connect to appsChanged).
    void refreshCatalog();

private:
    AppSearchService *m_apps;
    QVariantList m_catalog; // all apps, unfiltered
    QVariantList m_items;   // current filtered/ranked view
    QString m_query;
};

// Built-in commands surfaced in root search (e.g. Clipboard History). Backed
// by CommandRegistry so extension-provided commands can join the same list
// later without this section changing.
class CommandRegistry;
class Command;
class Navigation;

class CommandsSection : public SectionSource {
public:
    explicit CommandsSection(const CommandRegistry *registry);

    // Set once Navigation exists (see RootSearchModel::setNavigation).
    void setNavigation(Navigation *navigation) { m_navigation = navigation; }

    QString sectionName() const override { return QStringLiteral("Commands"); }
    int count() const override { return static_cast<int>(m_filtered.size()); }
    QVariantMap item(int index) const override;
    void activate(int index) override;
    void setFilter(const QString &query) override;

private:
    const CommandRegistry *m_registry;
    Navigation *m_navigation;
    std::vector<std::shared_ptr<Command>> m_filtered;
};

class SnippetSection : public SectionSource {
public:
    explicit SnippetSection(SnippetService *snippets);

    QString sectionName() const override { return QStringLiteral("Snippets"); }
    int count() const override { return static_cast<int>(m_items.size()); }
    QVariantMap item(int index) const override { return m_items.value(index).toMap(); }
    void activate(int index) override;
    void setFilter(const QString &query) override;
    std::unique_ptr<ActionPanelState> actionsFor(int index) const override;

private:
    SnippetService *m_snippets;
    QVariantList m_items;
};

// Async: Rust USN index when ready, Windows Search fallback while it warms up.
class FileSection : public QObject, public SectionSource {
    Q_OBJECT

public:
    FileSection(FileIndexService *index, WinFileSearchService *fallback,
                QObject *parent = nullptr);

    QString sectionName() const override { return QStringLiteral("Files"); }
    int count() const override { return static_cast<int>(m_items.size()); }
    QVariantMap item(int index) const override;
    void activate(int index) override;
    void setFilter(const QString &query) override;
    std::unique_ptr<ActionPanelState> actionsFor(int index) const override;

    // Reports async search activity (drives the loading bar).
    void setOnLoading(std::function<void(bool)> callback) {
        m_onLoading = std::move(callback);
    }

private:
    void onSearchFinished();

    FileIndexService *m_index;
    WinFileSearchService *m_fallback;
    QFutureWatcher<QVariantList> m_watcher;
    QVariantList m_items;
    QString m_query;         // latest query typed
    QString m_inFlightQuery; // query the running future belongs to
    std::function<void(bool)> m_onLoading;
};
