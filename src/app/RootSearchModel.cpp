#include "RootSearchModel.hpp"

#include "PerfTrace.hpp"
#include "RootSearchSections.hpp"
#include "core/nav/Navigation.hpp"
#include "views/ClipboardHistoryViewHost.hpp"
#include "services/apps/AppSearchService.hpp"
#include "services/calculator/CalculatorService.hpp"
#include "services/clipboard/ClipboardService.hpp"
#include "services/files/FileIndexService.hpp"
#include "services/files/WinFileSearchService.hpp"
#include "services/snippet/SnippetService.hpp"

#include <QElapsedTimer>

namespace {

QElapsedTimer &searchClock() {
    static QElapsedTimer timer = [] {
        QElapsedTimer t;
        t.start();
        return t;
    }();
    return timer;
}

} // namespace

RootSearchModel::RootSearchModel(QObject *parent)
    : SectionListModel(parent),
      m_appSearch(std::make_unique<AppSearchService>(this)),
      m_fileSearch(std::make_unique<WinFileSearchService>(this)),
      m_fileIndex(std::make_unique<FileIndexService>(this)),
      m_calculator(std::make_unique<CalculatorService>(this)),
      m_clipboard(std::make_unique<ClipboardService>(this)),
      m_snippets(std::make_unique<SnippetService>(this)) {

    m_calcSection = std::make_unique<CalculatorSection>(m_calculator.get());
    m_appSection = std::make_unique<AppSection>(m_appSearch.get());
    m_builtinSection = std::make_unique<BuiltinCommandsSection>([this](const QString &commandId) {
        if (!m_navigation)
            return;
        if (commandId == QLatin1String("clipboard-history"))
            m_navigation->push(new ClipboardHistoryViewHost(m_clipboard.get()));
    });
    m_snippetSection = std::make_unique<SnippetSection>(m_snippets.get());
    m_fileSection = std::make_unique<FileSection>(m_fileIndex.get(), m_fileSearch.get(), this);

    m_fileSection->setOnLoading([this](bool loading) { setLoading(loading); });

    // Display order.
    addSource(m_calcSection.get());
    addSource(m_appSection.get());
    addSource(m_builtinSection.get());
    addSource(m_snippetSection.get());
    addSource(m_fileSection.get());

    connect(m_appSearch.get(), &AppSearchService::appsChanged, this,
            [this] { m_appSection->refreshCatalog(); });
    connect(m_snippets.get(), &SnippetService::snippetsChanged, this,
            [this] { applyFilter(m_query); });

    applyFilter({});
}

RootSearchModel::~RootSearchModel() = default;

void RootSearchModel::setQuery(const QString &query) {
    if (m_query == query)
        return;
    m_query = query;
    m_queryStartedNs = searchClock().nsecsElapsed();
    emit queryChanged();

    applyFilter(query);

    if (m_queryStartedNs > 0) {
        PerfTrace::log(QStringLiteral("query_to_instant_results_ms=%1 rows=%2")
                           .arg((searchClock().nsecsElapsed() - m_queryStartedNs) / 1e6, 0, 'f', 2)
                           .arg(rowCount()));
    }
}

void RootSearchModel::setLoading(bool loading) {
    if (m_isLoading == loading)
        return;
    m_isLoading = loading;
    emit isLoadingChanged();
}
