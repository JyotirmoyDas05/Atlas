#pragma once

#include <QFuture>
#include <QObject>
#include <QString>
#include <QVariantList>

// Bridge to the Rust indexer (atlas_indexer.dll): full-volume file index with
// nucleo fuzzy search and frecency ranking. Falls back gracefully: isReady()
// stays false until the engine has an index (snapshot or fresh build), letting
// callers use Windows Search in the meantime.
class FileIndexService : public QObject {
    Q_OBJECT

public:
    explicit FileIndexService(QObject *parent = nullptr);

    // True once the Rust engine has a searchable index loaded.
    bool isReady() const;

    // Engine status JSON: state/source/file_count/build_ms (for logging/UI).
    QString statusJson() const;

    QFuture<QVariantList> searchAsync(const QString &query, int limit = 25) const;

    // Feed the frecency store when the user opens a result.
    void recordOpen(const QString &path);

private:
    static QVariantList runQuery(const QString &query, int limit);
};
