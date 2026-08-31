#pragma once

// The root palette model: owns the services and the four section sources
// (calculator, apps, snippets, files) and exposes one sectioned list to QML
// under the `searchModel` context property.

#include "core/commands/CommandRegistry.hpp"
#include "core/list/SectionListModel.hpp"

#include <memory>

class AppSearchService;
class WinFileSearchService;
class FileIndexService;
class CalculatorService;
class ClipboardService;
class SnippetService;
class CalculatorSection;
class AppSection;
class CommandsSection;
class SnippetSection;
class FileSection;
class Navigation;

class RootSearchModel : public SectionListModel {
    Q_OBJECT
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

public:
    explicit RootSearchModel(QObject *parent = nullptr);
    ~RootSearchModel() override;

    QString query() const { return m_query; }
    void setQuery(const QString &query);
    bool isLoading() const { return m_isLoading; }

    // Enables built-in commands that push views (not owned).
    void setNavigation(Navigation *navigation);

signals:
    void queryChanged();
    void isLoadingChanged();

private:
    void setLoading(bool loading);

    QString m_query;
    bool m_isLoading = false;
    qint64 m_queryStartedNs = 0;

    std::unique_ptr<AppSearchService> m_appSearch;
    std::unique_ptr<WinFileSearchService> m_fileSearch;
    std::unique_ptr<FileIndexService> m_fileIndex;
    std::unique_ptr<CalculatorService> m_calculator;
    std::unique_ptr<ClipboardService> m_clipboard;
    std::unique_ptr<SnippetService> m_snippets;

    CommandRegistry m_commandRegistry;

    std::unique_ptr<CalculatorSection> m_calcSection;
    std::unique_ptr<AppSection> m_appSection;
    std::unique_ptr<CommandsSection> m_commandsSection;
    std::unique_ptr<SnippetSection> m_snippetSection;
    std::unique_ptr<FileSection> m_fileSection;
    Navigation *m_navigation = nullptr;
};
