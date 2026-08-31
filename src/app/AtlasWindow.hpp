#pragma once

#include <QQuickWindow>

class AtlasWindow : public QQuickWindow {
    Q_OBJECT

public:
    explicit AtlasWindow(QWindow *parent = nullptr);
    ~AtlasWindow() override;

    void toggleVisibility();
    void enableWindowsMica();

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    void focusOutEvent(QFocusEvent *ev) override;

private:
    void registerGlobalHotkey();
    void unregisterGlobalHotkey();

    bool m_hotkeyRegistered{false};
};
