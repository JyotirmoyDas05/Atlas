#include "ClipboardService.hpp"
#include <QApplication>
#include <QClipboard>
#include <QCryptographicHash>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QUuid>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>

static constexpr int MAX_HISTORY = 500;

ClipboardService::ClipboardService(QObject *parent)
    : QObject(parent) {
    initDb();

    if (auto *cb = QApplication::clipboard()) {
        connect(cb, &QClipboard::dataChanged, this, &ClipboardService::onClipboardChanged);
    }
}

ClipboardService::~ClipboardService() {
    if (m_db.isOpen()) m_db.close();
}

void ClipboardService::initDb() {
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                            + "/atlas";
    QDir().mkpath(dataDir);

    const QString dbPath = dataDir + "/clipboard.db";
    m_db = QSqlDatabase::addDatabase("QSQLITE", "clipboard");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "[Atlas] Clipboard DB open failed:" << m_db.lastError().text();
        return;
    }

    QSqlQuery q(m_db);
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS entries (
            id        TEXT PRIMARY KEY,
            text      TEXT NOT NULL,
            preview   TEXT,
            mime_type TEXT,
            pinned    INTEGER DEFAULT 0,
            copied_at INTEGER NOT NULL
        )
    )");
    q.exec("CREATE INDEX IF NOT EXISTS idx_copied_at ON entries(copied_at DESC)");
}

void ClipboardService::setMonitoring(bool v) {
    if (m_monitoring == v) return;
    m_monitoring = v;
    emit monitoringChanged(v);
}

void ClipboardService::onClipboardChanged() {
    if (!m_monitoring) return;

    const auto *cb = QApplication::clipboard();
    if (!cb) return;

    const QString text = cb->text().trimmed();
    if (text.isEmpty()) return;

    // Dedup: skip if same as last captured
    const QByteArray hash = QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Md5);
    const QString hashStr = QString::fromLatin1(hash.toHex());
    if (hashStr == m_lastHash) return;
    m_lastHash = hashStr;

    ClipboardEntry entry;
    entry.id       = generateId();
    entry.text     = text;
    entry.preview  = makePreview(text);
    entry.mimeType = "text/plain";
    entry.copiedAt = QDateTime::currentDateTimeUtc();

    insertEntry(entry);

    // Prune beyond max
    QSqlQuery prune(m_db);
    prune.prepare(R"(
        DELETE FROM entries WHERE id NOT IN (
            SELECT id FROM entries ORDER BY pinned DESC, copied_at DESC LIMIT :limit
        )
    )");
    prune.bindValue(":limit", MAX_HISTORY);
    prune.exec();

    emit historyChanged();
}

bool ClipboardService::insertEntry(const ClipboardEntry &e) {
    QSqlQuery q(m_db);
    q.prepare(R"(
        INSERT OR REPLACE INTO entries(id, text, preview, mime_type, pinned, copied_at)
        VALUES(:id, :text, :preview, :mime, :pinned, :ts)
    )");
    q.bindValue(":id",      e.id);
    q.bindValue(":text",    e.text);
    q.bindValue(":preview", e.preview);
    q.bindValue(":mime",    e.mimeType);
    q.bindValue(":pinned",  e.pinned ? 1 : 0);
    q.bindValue(":ts",      e.copiedAt.toSecsSinceEpoch());
    if (!q.exec()) {
        qWarning() << "[Atlas] Clipboard insert failed:" << q.lastError().text();
        return false;
    }
    return true;
}

QVariantList ClipboardService::history(const QString &filter, int limit) const {
    QVariantList out;
    if (!m_db.isOpen()) return out;

    QSqlQuery q(m_db);
    if (filter.isEmpty()) {
        q.prepare("SELECT id, preview, mime_type, pinned, copied_at FROM entries "
                  "ORDER BY pinned DESC, copied_at DESC LIMIT :limit");
        q.bindValue(":limit", limit);
    } else {
        q.prepare("SELECT id, preview, mime_type, pinned, copied_at FROM entries "
                  "WHERE text LIKE :f OR preview LIKE :f "
                  "ORDER BY pinned DESC, copied_at DESC LIMIT :limit");
        q.bindValue(":f",     "%" + filter + "%");
        q.bindValue(":limit", limit);
    }

    if (!q.exec()) return out;

    while (q.next()) {
        const QDateTime dt = QDateTime::fromSecsSinceEpoch(q.value(4).toLongLong(), Qt::UTC);
        out.append(QVariantMap{
            {"id",       q.value(0).toString()},
            {"title",    q.value(1).toString()},
            {"subtitle", dt.toLocalTime().toString("MMM d, hh:mm")},
            {"mimeType", q.value(2).toString()},
            {"pinned",   q.value(3).toBool()},
            {"type",     "Clipboard"},
            {"section",  "Clipboard History"},
        });
    }
    return out;
}

bool ClipboardService::pinEntry(const QString &id, bool pinned) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE entries SET pinned = :p WHERE id = :id");
    q.bindValue(":p",  pinned ? 1 : 0);
    q.bindValue(":id", id);
    if (!q.exec()) return false;
    emit historyChanged();
    return true;
}

bool ClipboardService::removeEntry(const QString &id) {
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM entries WHERE id = :id");
    q.bindValue(":id", id);
    if (!q.exec()) return false;
    emit historyChanged();
    return true;
}

bool ClipboardService::clearAll() {
    QSqlQuery q(m_db);
    if (!q.exec("DELETE FROM entries WHERE pinned = 0")) return false;
    emit historyChanged();
    return true;
}

bool ClipboardService::copyEntry(const QString &id) {
    QSqlQuery q(m_db);
    q.prepare("SELECT text FROM entries WHERE id = :id");
    q.bindValue(":id", id);
    if (!q.exec() || !q.next()) return false;

    if (auto *cb = QApplication::clipboard()) {
        cb->setText(q.value(0).toString());
        return true;
    }
    return false;
}

QString ClipboardService::generateId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString ClipboardService::makePreview(const QString &text) {
    const QString trimmed = text.simplified();
    if (trimmed.length() <= 200) return trimmed;
    return trimmed.left(197) + "…";
}
