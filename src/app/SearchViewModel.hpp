#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QString>
#include <QFutureWatcher>
#include <memory>

class AppSearchService;
class WinFileSearchService;
class FileIndexService;
class CalculatorService;
class ClipboardService;
class SnippetService;

class SearchViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged)
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedIndexChanged)
    Q_PROPERTY(QString selectedTitle READ selectedTitle NOTIFY selectedIndexChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

public:
    explicit SearchViewModel(QObject *parent = nullptr);
    ~SearchViewModel() override;

    QString query() const { return m_query; }
    void setQuery(const QString &query);

    QVariantList results() const { return m_results; }

    int selectedIndex() const { return m_selectedIndex; }
    void setSelectedIndex(int index);

    QString selectedTitle() const;
    bool isLoading() const { return m_isLoading; }

    Q_INVOKABLE void executeSelected(int index);
    Q_INVOKABLE int nextSelectableIndex(int current, int direction) const;
    Q_INVOKABLE void activateSelected();

signals:
    void queryChanged();
    void resultsChanged();
    void selectedIndexChanged();
    void isLoadingChanged();
    void itemActivated(int index, const QVariantMap &item);

private slots:
    void onFileSearchFinished();

private:
    void performSearch();
    void setLoading(bool loading);
    QVariantList assembleResults() const;
    void publishResults(bool preserveSelection);

    QString m_query;
    QVariantList m_results;
    int m_selectedIndex = 0;
    bool m_isLoading = false;

    std::unique_ptr<AppSearchService> m_appSearch;
    std::unique_ptr<WinFileSearchService> m_fileSearch;
    std::unique_ptr<FileIndexService> m_fileIndex;
    std::unique_ptr<CalculatorService> m_calculator;
    std::unique_ptr<ClipboardService> m_clipboard;
    std::unique_ptr<SnippetService> m_snippets;

    QFutureWatcher<QVariantList> m_fileSearchWatcher;
    QVariantList m_fileResults;
    qint64 m_queryStartedNs = 0;
};
