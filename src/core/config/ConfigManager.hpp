#pragma once

#include "ConfigValue.hpp"

#include <QFileSystemWatcher>
#include <QObject>
#include <QString>
#include <QTimer>

// Loads settings.jsonc, merges it over defaults, watches the file for
// changes (debounced) and live-reloads. Bootstraps a commented default file
// on first run so there's something for the user to discover and edit.
class ConfigManager : public QObject {
    Q_OBJECT

public:
    explicit ConfigManager(QString filePath, QObject *parent = nullptr);

    const ConfigValue &current() const { return m_current; }

signals:
    // Emitted after every successful (re)load, including the first.
    void configChanged(const ConfigValue &next, const ConfigValue &prev);

private:
    void load();
    void writeDefaultFileIfMissing() const;
    void onFileChanged();

    QString m_filePath;
    ConfigValue m_current;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
};
