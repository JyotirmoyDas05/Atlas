#pragma once

#include <QObject>
#include <QQuickWindow>
#include <QAbstractNativeEventFilter>
#include <thread>
#include <atomic>

class AtlasNativeHost : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT

public:
    explicit AtlasNativeHost(QQuickWindow *window, QObject *parent = nullptr);
    ~AtlasNativeHost() override;

    void applyWindowsChrome();
    Q_INVOKABLE void toggleVisibility();
    Q_INVOKABLE void showWindow();
    Q_INVOKABLE void hideWindow();

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void startKeyboardHook();
    void stopKeyboardHook();

    QQuickWindow *m_window{nullptr};

    // Low-level keyboard hook thread members
    std::thread m_hookThread;
    unsigned long m_hookThreadId{0};
    std::atomic<bool> m_hookInstalled{false};
};
