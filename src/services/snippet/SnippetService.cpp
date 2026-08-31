#include "SnippetService.hpp"
#include <QApplication>
#include <QClipboard>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QUuid>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>

SnippetService::SnippetService(QObject *parent)
    : QObject(parent) {
    initDb();
}

SnippetService::~SnippetService() {
    if (m_db.isOpen()) m_db.close();
}

void SnippetService::initDb() {
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/atlas";
    QDir().mkpath(dataDir);

    const QString dbPath = dataDir + "/snippets.db";
    m_db = QSqlDatabase::addDatabase("QSQLITE", "snippets");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "[Atlas] Snippet DB open failed:" << m_db.lastError().text();
        return;
    }

    QSqlQuery q(m_db);
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS snippets (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            keyword TEXT,
            text TEXT NOT NULL
        )
    )");

    // Check count to see if we should seed default snippets
    if (q.exec("SELECT COUNT(*) FROM snippets") && q.next()) {
        if (q.value(0).toInt() == 0) {
            seedDefaults();
        }
    }
}

void SnippetService::seedDefaults() {
    addSnippet("Shrug", "!shrug", "¯\\_(ツ)_/¯");
    addSnippet("Tableflip", "!flip", "(╯°□°)╯︵ ┻━┻");
    addSnippet("Email Signature", "!sig", "Best regards,\nAtlas User");
}

QVariantList SnippetService::search(const QString &query) const {
    QVariantList out;
    if (!m_db.isOpen()) return out;

    QSqlQuery q(m_db);
    if (query.isEmpty()) {
        q.prepare("SELECT id, name, keyword, text FROM snippets ORDER BY name ASC");
    } else {
        q.prepare("SELECT id, name, keyword, text FROM snippets WHERE name LIKE :q OR keyword LIKE :q OR text LIKE :q ORDER BY name ASC");
        q.bindValue(":q", "%" + query + "%");
    }

    if (!q.exec()) return out;

    while (q.next()) {
        out.append(QVariantMap{
            {"id",       q.value(0).toString()},
            {"title",    q.value(1).toString()},
            {"alias",    q.value(2).toString()},
            {"subtitle", q.value(3).toString()},
            {"type",     "Snippet"},
            {"section",  "Snippets"},
        });
    }
    return out;
}

bool SnippetService::addSnippet(const QString &name, const QString &keyword, const QString &text) {
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO snippets (id, name, keyword, text) VALUES (:id, :name, :kw, :text)");
    q.bindValue(":id", QUuid::createUuid().toString(QUuid::WithoutBraces));
    q.bindValue(":name", name);
    q.bindValue(":kw", keyword);
    q.bindValue(":text", text);

    if (!q.exec()) return false;
    emit snippetsChanged();
    return true;
}

bool SnippetService::removeSnippet(const QString &id) {
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM snippets WHERE id = :id");
    q.bindValue(":id", id);

    if (!q.exec()) return false;
    emit snippetsChanged();
    return true;
}

bool SnippetService::updateSnippet(const QString &id, const QString &name, const QString &keyword, const QString &text) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE snippets SET name = :name, keyword = :kw, text = :text WHERE id = :id");
    q.bindValue(":name", name);
    q.bindValue(":kw", keyword);
    q.bindValue(":text", text);
    q.bindValue(":id", id);

    if (!q.exec()) return false;
    emit snippetsChanged();
    return true;
}

bool SnippetService::pasteSnippet(const QString &id) const {
    QSqlQuery q(m_db);
    q.prepare("SELECT text FROM snippets WHERE id = :id");
    q.bindValue(":id", id);

    if (!q.exec() || !q.next()) return false;

    if (auto *cb = QApplication::clipboard()) {
        cb->setText(q.value(0).toString());
        return true;
    }
    return false;
}
