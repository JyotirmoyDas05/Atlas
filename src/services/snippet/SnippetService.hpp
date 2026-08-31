#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QString>
#include <QtSql/QSqlDatabase>

struct SnippetEntry {
    QString id;
    QString name;
    QString keyword;
    QString text;
};

class SnippetService : public QObject {
    Q_OBJECT

public:
    explicit SnippetService(QObject *parent = nullptr);
    ~SnippetService() override;

    QVariantList search(const QString &query = {}) const;
    bool addSnippet(const QString &name, const QString &keyword, const QString &text);
    bool removeSnippet(const QString &id);
    bool updateSnippet(const QString &id, const QString &name,
                       const QString &keyword, const QString &text);
    bool pasteSnippet(const QString &id) const;

signals:
    void snippetsChanged();

private:
    void initDb();
    void seedDefaults();

    QSqlDatabase m_db;
};
