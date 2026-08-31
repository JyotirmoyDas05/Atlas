#include "FileIndexService.hpp"
#include "ShellIcons.hpp"
#include "app/PerfTrace.hpp"

#include <QtConcurrent/QtConcurrentRun>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

// C FFI surface of atlas_indexer.dll (see src/indexer/src/lib.rs)
extern "C" {
int atlas_indexer_start(const char *config_json);
char *atlas_indexer_search_json(const char *query, unsigned int limit);
char *atlas_indexer_status_json();
void atlas_indexer_record_open(const char *path);
void atlas_free_string(char *ptr);
}

namespace {

QString takeRustString(char *ptr) {
    if (!ptr)
        return {};
    const QString s = QString::fromUtf8(ptr);
    atlas_free_string(ptr);
    return s;
}

} // namespace

FileIndexService::FileIndexService(QObject *parent) : QObject(parent) {
    const QString dataDir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/atlas/index";
    QDir().mkpath(dataDir);

    const QJsonObject config{{"data_dir", QDir::toNativeSeparators(dataDir)}};
    const QByteArray json = QJsonDocument(config).toJson(QJsonDocument::Compact);
    const int rc = atlas_indexer_start(json.constData());
    PerfTrace::log(QStringLiteral("index_start rc=%1").arg(rc));
}

QString FileIndexService::statusJson() const {
    return takeRustString(atlas_indexer_status_json());
}

bool FileIndexService::isReady() const {
    const QJsonObject status = QJsonDocument::fromJson(statusJson().toUtf8()).object();
    return status.value("state").toString() == QLatin1String("ready") &&
           status.value("file_count").toInteger() > 0;
}

void FileIndexService::recordOpen(const QString &path) {
    atlas_indexer_record_open(path.toUtf8().constData());
}

// static
QVariantList FileIndexService::runQuery(const QString &query, int limit) {
    QElapsedTimer timer;
    timer.start();

    const QString json = takeRustString(
        atlas_indexer_search_json(query.toUtf8().constData(), static_cast<unsigned int>(limit)));
    const qint64 searchUs = timer.nsecsElapsed() / 1000;

    QVariantList results;
    const QJsonArray items = QJsonDocument::fromJson(json.toUtf8()).array();
    results.reserve(items.size());
    for (const auto &value : items) {
        const QJsonObject item = value.toObject();
        const QString path = item.value("path").toString();
        const bool isDir = item.value("is_dir").toBool();
        results.append(QVariantMap{
            {"title", item.value("title").toString()},
            {"subtitle", path},
            {"path", path},
            {"icon", ShellIcons::iconUrlFor(path, isDir)},
            {"type", isDir ? "Folder" : "File"},
            {"section", "Files"},
        });
    }

    PerfTrace::log(QStringLiteral("index_search query_len=%1 hits=%2 rust_us=%3 total_us=%4")
                       .arg(query.length())
                       .arg(results.size())
                       .arg(searchUs)
                       .arg(timer.nsecsElapsed() / 1000));
    return results;
}

QFuture<QVariantList> FileIndexService::searchAsync(const QString &query, int limit) const {
    return QtConcurrent::run([q = query, limit]() { return runQuery(q, limit); });
}
