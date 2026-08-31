#include "RootSearchSections.hpp"

#include "core/fuzzy/Fuzzy.hpp"
#include "services/apps/AppSearchService.hpp"
#include "services/calculator/CalculatorService.hpp"
#include "services/files/FileIndexService.hpp"
#include "services/files/WinFileSearchService.hpp"
#include "services/snippet/SnippetService.hpp"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QUrl>
#include <algorithm>
#include <array>

namespace {

constexpr int MAX_APP_RESULTS = 15;

QString concisePath(const QString &fullPath) {
    const QString home = QDir::homePath();
    if (!home.isEmpty() && fullPath.startsWith(home, Qt::CaseInsensitive))
        return "~" + fullPath.mid(home.length());
    return fullPath;
}

} // namespace

// ---------------------------------------------------------------------------
// Calculator
// ---------------------------------------------------------------------------

CalculatorSection::CalculatorSection(CalculatorService *calculator)
    : m_calculator(calculator) {}

void CalculatorSection::setFilter(const QString &query) {
    m_items.clear();
    if (!m_calculator->looksLikeMath(query))
        return;
    const CalcResult res = m_calculator->evaluate(query);
    if (!res.ok)
        return;
    m_items.append(QVariantMap{
        {"itemId", "calculator"},
        {"isCalculator", true},
        {"title", res.answer},
        {"subtitle", res.question},
        {"calcQuestion", res.question},
        {"calcQuestionUnit", res.questionUnit},
        {"calcAnswer", res.answer},
        {"calcAnswerUnit", res.answerUnit},
        {"type", "Calculator"},
    });
}

void CalculatorSection::activate(int index) {
    if (auto *cb = QApplication::clipboard())
        cb->setText(m_items.value(index).toMap().value("calcAnswer").toString());
}

// ---------------------------------------------------------------------------
// Applications
// ---------------------------------------------------------------------------

AppSection::AppSection(AppSearchService *apps) : m_apps(apps) {
    refreshCatalog();
}

void AppSection::refreshCatalog() {
    m_catalog = m_apps->all();
    setFilter(m_query);
    notifyChanged(/*preserveSelection=*/true);
}

void AppSection::setFilter(const QString &query) {
    m_query = query;
    m_items.clear();

    if (query.isEmpty()) {
        m_items = m_catalog;
        for (auto &v : m_items) {
            QVariantMap m = v.toMap();
            m["subtitle"] = QString(); // browse view: clean names, no paths
            m["itemId"] = "app:" + m.value("path").toString();
            v = m;
        }
        return;
    }

    const fuzzy::Query fq(query);
    struct Scored {
        QVariantMap item;
        int score;
    };
    std::vector<Scored> scored;
    scored.reserve(m_catalog.size());

    for (const auto &v : m_catalog) {
        QVariantMap m = v.toMap();
        const QString title = m.value("title").toString();
        const std::array fields = {fuzzy::Field{.text = title, .weight = 1.0f}};
        const auto qs = fuzzy::scoreFields(fields, fq);
        if (!qs.matched || qs.quality < fuzzy::MIN_QUALITY)
            continue;
        m["subtitle"] = QString();
        m["itemId"] = "app:" + m.value("path").toString();
        scored.push_back({std::move(m), qs.score});
    }

    std::stable_sort(scored.begin(), scored.end(),
                     [](const Scored &a, const Scored &b) { return a.score > b.score; });
    if (scored.size() > MAX_APP_RESULTS)
        scored.resize(MAX_APP_RESULTS);

    for (auto &s : scored)
        m_items.append(std::move(s.item));
}

void AppSection::activate(int index) {
    const QString path = m_items.value(index).toMap().value("path").toString();
    if (!path.isEmpty())
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

// ---------------------------------------------------------------------------
// Built-in commands
// ---------------------------------------------------------------------------

BuiltinCommandsSection::BuiltinCommandsSection(
    std::function<void(const QString &)> onActivate)
    : m_onActivate(std::move(onActivate)) {
    m_commands = {
        {.id = "clipboard-history", .title = "Clipboard History"},
    };
}

void BuiltinCommandsSection::setFilter(const QString &query) {
    m_items.clear();
    const fuzzy::Query fq(query);

    for (const auto &cmd : m_commands) {
        if (!query.isEmpty()) {
            const std::array fields = {fuzzy::Field{.text = cmd.title, .weight = 1.0f}};
            const auto qs = fuzzy::scoreFields(fields, fq);
            if (!qs.matched || qs.quality < fuzzy::MIN_QUALITY)
                continue;
        }
        m_items.append(QVariantMap{
            {"itemId", "builtin:" + cmd.id},
            {"title", cmd.title},
            {"type", "Built-in"},
            {"commandId", cmd.id},
        });
    }
}

void BuiltinCommandsSection::activate(int index) {
    if (m_onActivate)
        m_onActivate(m_items.value(index).toMap().value("commandId").toString());
}

// ---------------------------------------------------------------------------
// Snippets
// ---------------------------------------------------------------------------

SnippetSection::SnippetSection(SnippetService *snippets) : m_snippets(snippets) {}

void SnippetSection::setFilter(const QString &query) {
    m_items.clear();
    if (query.isEmpty())
        return;
    m_items = m_snippets->search(query);
    for (auto &v : m_items) {
        QVariantMap m = v.toMap();
        m["itemId"] = "snippet:" + m.value("id").toString();
        v = m;
    }
}

void SnippetSection::activate(int index) {
    m_snippets->pasteSnippet(m_items.value(index).toMap().value("id").toString());
}

// ---------------------------------------------------------------------------
// Files (async)
// ---------------------------------------------------------------------------

FileSection::FileSection(FileIndexService *index, WinFileSearchService *fallback,
                         QObject *parent)
    : QObject(parent), m_index(index), m_fallback(fallback) {
    connect(&m_watcher, &QFutureWatcher<QVariantList>::finished, this,
            &FileSection::onSearchFinished);
}

QVariantMap FileSection::item(int index) const {
    QVariantMap m = m_items.value(index).toMap();
    m["itemId"] = "file:" + m.value("path").toString();
    m["subtitle"] = concisePath(m.value("subtitle").toString());
    return m;
}

void FileSection::setFilter(const QString &query) {
    m_query = query;

    if (query.isEmpty()) {
        m_items.clear();
        if (m_onLoading)
            m_onLoading(false);
        return;
    }

    m_inFlightQuery = query;
    if (m_onLoading)
        m_onLoading(true);
    if (m_index->isReady())
        m_watcher.setFuture(m_index->searchAsync(query, 25));
    else if (m_fallback->isAvailable())
        m_watcher.setFuture(m_fallback->searchAsync(query, 25));
    else if (m_onLoading)
        m_onLoading(false);
}

void FileSection::onSearchFinished() {
    // Stale guard: the user has typed since this search started; its results
    // would be wrong AND a fresher future is (or will be) on the way.
    if (m_inFlightQuery != m_query)
        return;

    if (m_onLoading)
        m_onLoading(false);
    m_items = m_watcher.result();
    notifyChanged(/*preserveSelection=*/true);
}

void FileSection::activate(int index) {
    const QVariantMap m = m_items.value(index).toMap();
    const QString path = m.value("path").toString();
    if (path.isEmpty())
        return;
    m_index->recordOpen(path); // frecency: boost next time
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}
