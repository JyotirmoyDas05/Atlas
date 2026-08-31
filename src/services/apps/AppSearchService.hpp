#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QString>
#include <QFileSystemWatcher>
#include <QTimer>
#include <vector>

struct AppEntry {
    QString name;
    QString path;           // resolved target path
    QString lnkPath;        // path to .lnk file, if any
    QString description;
    QString iconUrl;        // file:/// URL to extracted PNG icon
    QString category = "Application"; // Game, Developer, Browser, Design, Productivity, System, Application
    bool isDirectory = false;
};

class AppSearchService : public QObject {
    Q_OBJECT

public:
    explicit AppSearchService(QObject *parent = nullptr);

    // Returns fuzzy-filtered, scored list as QVariantMaps
    QVariantList search(const QString &query) const;
    QVariantList all() const;

signals:
    void appsChanged();

private:
    void refresh();
    void enumerateDirectory(const QString &dir);
    void extractAndCacheIcon(AppEntry &entry);
    static int fuzzyScore(const QString &name, const QString &query);

    std::vector<AppEntry> m_apps;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
    QString m_iconCacheDir;
};
