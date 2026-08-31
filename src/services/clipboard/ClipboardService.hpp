#pragma once

#include <QObject>
#include <QClipboard>
#include <QVariantList>
#include <QVariantMap>
#include <QString>
#include <QDateTime>
#include <QtSql/QSqlDatabase>

struct ClipboardEntry {
    QString id;
    QString text;
    QString preview;    // first 200 chars
    QString mimeType;
    bool pinned = false;
    QDateTime copiedAt;
};

class ClipboardService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool monitoring READ monitoring WRITE setMonitoring NOTIFY monitoringChanged)

public:
    explicit ClipboardService(QObject *parent = nullptr);
    ~ClipboardService() override;

    bool monitoring() const { return m_monitoring; }
    void setMonitoring(bool v);

    QVariantList history(const QString &filter = {}, int limit = 200) const;
    bool pinEntry(const QString &id, bool pinned);
    bool removeEntry(const QString &id);
    bool clearAll();
    bool copyEntry(const QString &id);

signals:
    void monitoringChanged(bool v);
    void historyChanged();

private slots:
    void onClipboardChanged();

private:
    void initDb();
    bool insertEntry(const ClipboardEntry &e);
    static QString generateId();
    static QString makePreview(const QString &text);

    QSqlDatabase m_db;
    bool m_monitoring = true;
    QString m_lastHash;
};
