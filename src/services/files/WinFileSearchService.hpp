#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QString>
#include <QFuture>

struct FileResult {
    QString path;
    QString name;
    QString mimeType;
    bool isDirectory = false;
    int score = 0;
};

class WinFileSearchService : public QObject {
    Q_OBJECT

public:
    explicit WinFileSearchService(QObject *parent = nullptr);

    bool isAvailable() const;
    QFuture<QVariantList> searchAsync(const QString &query, int limit = 50) const;

private:
    static QVariantList runQuery(const QString &query, int limit);
    static bool winSearchAvailable();
};
