#include "SearchViewModel.hpp"
#include "PerfTrace.hpp"
#include "services/apps/AppSearchService.hpp"
#include "services/files/WinFileSearchService.hpp"
#include "services/files/FileIndexService.hpp"
#include "services/calculator/CalculatorService.hpp"
#include "services/clipboard/ClipboardService.hpp"
#include "services/snippet/SnippetService.hpp"

#include <QDebug>
#include <QDesktopServices>
#include <QElapsedTimer>
#include <QUrl>
#include <QProcess>
#include <QClipboard>
#include <QApplication>
#include <QDir>
#include <algorithm>

namespace {

QElapsedTimer &searchClock() {
    static QElapsedTimer timer = [] {
        QElapsedTimer t;
        t.start();
        return t;
    }();
    return timer;
}

QString formatConcisePath(const QString &fullPath) {
    const QString userHome = QDir::homePath();
    if (!userHome.isEmpty() && fullPath.startsWith(userHome, Qt::CaseInsensitive)) {
        return "~" + fullPath.mid(userHome.length());
    }
    return fullPath;
}

} // namespace

SearchViewModel::SearchViewModel(QObject *parent)
    : QObject(parent),
      m_appSearch(std::make_unique<AppSearchService>(this)),
      m_fileSearch(std::make_unique<WinFileSearchService>(this)),
      m_fileIndex(std::make_unique<FileIndexService>(this)),
      m_calculator(std::make_unique<CalculatorService>(this)),
      m_clipboard(std::make_unique<ClipboardService>(this)),
      m_snippets(std::make_unique<SnippetService>(this)) {

    connect(&m_fileSearchWatcher, &QFutureWatcher<QVariantList>::finished,
            this, &SearchViewModel::onFileSearchFinished);

    connect(m_appSearch.get(), &AppSearchService::appsChanged, this, &SearchViewModel::performSearch);
    connect(m_clipboard.get(), &ClipboardService::historyChanged, this, &SearchViewModel::performSearch);
    connect(m_snippets.get(), &SnippetService::snippetsChanged, this, &SearchViewModel::performSearch);

    performSearch();
}

SearchViewModel::~SearchViewModel() = default;

void SearchViewModel::setQuery(const QString &query) {
    if (m_query == query)
        return;
    m_query = query;
    m_queryStartedNs = searchClock().nsecsElapsed();
    emit queryChanged();
    performSearch();
}

void SearchViewModel::setSelectedIndex(int index) {
    if (m_results.isEmpty())
        index = -1;
    else
        index = std::clamp(index, 0, static_cast<int>(m_results.size()) - 1);

    if (m_selectedIndex == index)
        return;
    m_selectedIndex = index;
    emit selectedIndexChanged();
}

QString SearchViewModel::selectedTitle() const {
    if (m_selectedIndex < 0 || m_selectedIndex >= m_results.size())
        return {};
    return m_results[m_selectedIndex].toMap().value("title").toString();
}

void SearchViewModel::setLoading(bool loading) {
    if (m_isLoading == loading) return;
    m_isLoading = loading;
    emit isLoadingChanged();
}

int SearchViewModel::nextSelectableIndex(int current, int direction) const {
    const int count = m_results.size();
    if (count == 0) return -1;

    int next = current + direction;
    while (next >= 0 && next < count) {
        const QVariantMap item = m_results[next].toMap();
        if (!item.value("isSectionHeader").toBool()) {
            return next;
        }
        next += direction;
    }
    return current;
}

void SearchViewModel::executeSelected(int index) {
    if (index < 0 || index >= m_results.size())
        return;

    QVariantMap item = m_results[index].toMap();
    const QString type = item["type"].toString();
    const QString path = item["path"].toString();
    qDebug() << "[Atlas] Executing item:" << item["title"].toString() << "type:" << type;

    if (type == "Calculator") {
        if (auto *cb = QApplication::clipboard()) {
            cb->setText(item["calcAnswer"].toString());
        }
    } else if (type == "Application" || type == "File" || type == "Folder") {
        if (!path.isEmpty()) {
            if (type != "Application") {
                m_fileIndex->recordOpen(path); // frecency: boost next time
            }
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        }
    } else if (type == "Clipboard") {
        m_clipboard->copyEntry(item["id"].toString());
    } else if (type == "Snippet") {
        m_snippets->pasteSnippet(item["id"].toString());
    }

    emit itemActivated(index, item);
}

void SearchViewModel::activateSelected() {
    executeSelected(m_selectedIndex);
}

// Assemble the visible list from calculator + apps + snippets + cached file
// results. Called both on keystroke (stale file results) and when the async
// file search lands (fresh ones).
QVariantList SearchViewModel::assembleResults() const {
    QVariantList newResults;

    if (m_calculator->looksLikeMath(m_query)) {
        CalcResult res = m_calculator->evaluate(m_query);
        if (res.ok) {
            newResults.append(QVariantMap{
                {"isCalculator",   true},
                {"title",          res.answer},
                {"subtitle",       res.question},
                {"calcQuestion",   res.question},
                {"calcQuestionUnit", res.questionUnit},
                {"calcAnswer",     res.answer},
                {"calcAnswerUnit", res.answerUnit},
                {"type",           "Calculator"},
                {"accessory",      "Copy Result"}
            });
        }
    }

    QVariantList apps = m_appSearch->search(m_query);
    if (!apps.isEmpty()) {
        newResults.append(QVariantMap{
            {"isSectionHeader", true},
            {"title", "Applications"}
        });
        for (auto app : apps) {
            QVariantMap m = app.toMap();
            m["subtitle"] = ""; // Raycast style: clean app title with no path subtitle
            newResults.append(m);
        }
    }

    if (!m_query.isEmpty()) {
        QVariantList snips = m_snippets->search(m_query);
        if (!snips.isEmpty()) {
            newResults.append(QVariantMap{
                {"isSectionHeader", true},
                {"title", "Snippets"}
            });
            for (const auto &s : snips) {
                newResults.append(s);
            }
        }
    }

    if (!m_fileResults.isEmpty()) {
        newResults.append(QVariantMap{
            {"isSectionHeader", true},
            {"title", "Files"}
        });
        for (auto f : m_fileResults) {
            QVariantMap m = f.toMap();
            m["subtitle"] = formatConcisePath(m["subtitle"].toString());
            newResults.append(m);
        }
    }

    return newResults;
}

void SearchViewModel::publishResults(bool preserveSelection) {
    m_results = assembleResults();

    if (!preserveSelection || m_selectedIndex < 0 || m_selectedIndex >= m_results.size() ||
        m_results[m_selectedIndex].toMap().value("isSectionHeader").toBool()) {
        m_selectedIndex = nextSelectableIndex(-1, 1);
    }

    emit resultsChanged();
    emit selectedIndexChanged();
}

void SearchViewModel::performSearch() {
    // Kick off async file search: Rust index when ready, Windows Search fallback.
    if (!m_query.isEmpty()) {
        if (m_fileIndex->isReady()) {
            setLoading(true);
            m_fileSearchWatcher.setFuture(m_fileIndex->searchAsync(m_query, 25));
        } else if (m_fileSearch->isAvailable()) {
            setLoading(true);
            m_fileSearchWatcher.setFuture(m_fileSearch->searchAsync(m_query, 25));
        } else {
            setLoading(false);
            m_fileResults.clear();
        }
    } else {
        setLoading(false);
        m_fileResults.clear();
    }

    publishResults(/*preserveSelection=*/false);

    if (m_queryStartedNs > 0) {
        PerfTrace::log(QStringLiteral("query_to_instant_results_ms=%1 results=%2")
                           .arg((searchClock().nsecsElapsed() - m_queryStartedNs) / 1e6, 0, 'f', 2)
                           .arg(m_results.size()));
    }
}

void SearchViewModel::onFileSearchFinished() {
    setLoading(false);
    m_fileResults = m_fileSearchWatcher.result();

    // Keep the user's selection if they are already navigating.
    publishResults(/*preserveSelection=*/true);

    if (m_queryStartedNs > 0) {
        PerfTrace::log(QStringLiteral("query_to_file_results_ms=%1 files=%2")
                           .arg((searchClock().nsecsElapsed() - m_queryStartedNs) / 1e6, 0, 'f', 2)
                           .arg(m_fileResults.size()));
        m_queryStartedNs = 0;
    }
}
