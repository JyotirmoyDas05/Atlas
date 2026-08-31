#pragma once

#include <QObject>

class ConfigBridge : public QObject {
    Q_OBJECT

    Q_PROPERTY(int borderRounding READ borderRounding CONSTANT)
    Q_PROPERTY(int borderWidth READ borderWidth CONSTANT)
    Q_PROPERTY(qreal windowOpacity READ windowOpacity CONSTANT)
    Q_PROPERTY(int windowWidth READ windowWidth CONSTANT)
    Q_PROPERTY(int windowHeight READ windowHeight CONSTANT)

public:
    explicit ConfigBridge(QObject *parent = nullptr) : QObject(parent) {}

    int borderRounding() const { return 12; }
    int borderWidth() const { return 1; }
    qreal windowOpacity() const { return 0.94; }
    int windowWidth() const { return 760; }
    int windowHeight() const { return 480; }
};
