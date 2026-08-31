#pragma once

#include "core/config/ConfigManager.hpp"

#include <QObject>

// QML-facing view of ConfigManager's current value. Properties are NOTIFY-
// based (not CONSTANT) so editing settings.jsonc updates the running window
// live.
class ConfigBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(int borderRounding READ borderRounding NOTIFY changed)
    Q_PROPERTY(int borderWidth READ borderWidth NOTIFY changed)
    Q_PROPERTY(qreal windowOpacity READ windowOpacity NOTIFY changed)
    Q_PROPERTY(int windowWidth READ windowWidth NOTIFY changed)
    Q_PROPERTY(int windowHeight READ windowHeight NOTIFY changed)

public:
    explicit ConfigBridge(ConfigManager *manager, QObject *parent = nullptr)
        : QObject(parent), m_manager(manager) {
        connect(m_manager, &ConfigManager::configChanged, this, &ConfigBridge::changed);
    }

    int borderRounding() const { return m_manager->current().borderRounding; }
    int borderWidth() const { return m_manager->current().borderWidth; }
    qreal windowOpacity() const { return m_manager->current().windowOpacity; }
    int windowWidth() const { return m_manager->current().windowWidth; }
    int windowHeight() const { return m_manager->current().windowHeight; }

signals:
    void changed();

private:
    ConfigManager *m_manager;
};
