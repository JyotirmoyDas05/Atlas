#include "ConfigManager.hpp"

#include "Jsonc.hpp"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

constexpr const char *DEFAULT_TEMPLATE = R"JSONC({
  // Atlas window settings. Delete a line to fall back to its default;
  // changes here are picked up live, no restart needed.

  // Corner radius of the launcher window, in pixels.
  "borderRounding": 12,

  // Width of the window border, in pixels.
  "borderWidth": 1,

  // Window background opacity, 0.0 (fully transparent) to 1.0 (opaque).
  "windowOpacity": 0.94,

  // Launcher window size, in pixels.
  "windowWidth": 760,
  "windowHeight": 480
}
)JSONC";

PartialConfig parsePartial(const QJsonObject &obj) {
    PartialConfig p;
    if (obj.contains("borderRounding")) p.borderRounding = obj.value("borderRounding").toInt();
    if (obj.contains("borderWidth")) p.borderWidth = obj.value("borderWidth").toInt();
    if (obj.contains("windowOpacity")) p.windowOpacity = obj.value("windowOpacity").toDouble();
    if (obj.contains("windowWidth")) p.windowWidth = obj.value("windowWidth").toInt();
    if (obj.contains("windowHeight")) p.windowHeight = obj.value("windowHeight").toInt();
    return p;
}

} // namespace

ConfigManager::ConfigManager(QString filePath, QObject *parent)
    : QObject(parent), m_filePath(std::move(filePath)) {
    writeDefaultFileIfMissing();

    m_debounce.setSingleShot(true);
    m_debounce.setInterval(150);
    connect(&m_debounce, &QTimer::timeout, this, &ConfigManager::onFileChanged);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, &m_debounce, qOverload<>(&QTimer::start));
    m_watcher.addPath(m_filePath);

    load(); // initial load; emits configChanged(current, ConfigValue{}) below
}

void ConfigManager::writeDefaultFileIfMissing() const {
    if (QFile::exists(m_filePath))
        return;
    QDir().mkpath(QFileInfo(m_filePath).absolutePath());
    QFile file(m_filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        file.write(DEFAULT_TEMPLATE);
    else
        qWarning() << "[Config] Could not create default settings file at" << m_filePath
                   << ":" << file.errorString();
}

void ConfigManager::load() {
    const ConfigValue previous = m_current;

    QFile file(m_filePath);
    PartialConfig user;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QByteArray stripped = jsonc::stripComments(file.readAll());
        QJsonParseError error;
        const QJsonDocument doc = QJsonDocument::fromJson(stripped, &error);
        if (error.error == QJsonParseError::NoError && doc.isObject()) {
            user = parsePartial(doc.object());
        } else if (error.error != QJsonParseError::NoError) {
            qWarning() << "[Config] Failed to parse" << m_filePath << ":" << error.errorString();
        }
    }

    m_current = mergeWithUser(ConfigValue{}, user);
    emit configChanged(m_current, previous);
}

void ConfigManager::onFileChanged() {
    // Editors often replace-on-save (unlink + rewrite), which drops the
    // watched inode; re-add so future edits keep being observed.
    if (!m_watcher.files().contains(m_filePath) && QFile::exists(m_filePath))
        m_watcher.addPath(m_filePath);
    load();
}
