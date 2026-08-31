#include "AtlasWindow.hpp"
#include <QDebug>
#include <QGuiApplication>
#include <QScreen>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#endif

AtlasWindow::AtlasWindow(QWindow *parent)
    : QQuickWindow(parent) {
    setFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Dialog);
    setColor(QColor(18, 18, 24, 220));

    // Center window on active screen
    if (QScreen *primary = QGuiApplication::primaryScreen()) {
        QRect screenGeometry = primary->geometry();
        int width = 750;
        int height = 450;
        int x = (screenGeometry.width() - width) / 2;
        int y = (screenGeometry.height() - height) / 3;
        setGeometry(x, y, width, height);
    }

    enableWindowsMica();
    registerGlobalHotkey();
}

AtlasWindow::~AtlasWindow() {
    unregisterGlobalHotkey();
}

void AtlasWindow::enableWindowsMica() {
#ifdef Q_OS_WIN
    HWND hWnd = reinterpret_cast<HWND>(winId());
    if (hWnd) {
        DWM_SYSTEMBACKDROP_TYPE backdrop = DWMSBT_TRANSIENTWINDOW;
        DwmSetWindowAttribute(hWnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop, sizeof(backdrop));

        MARGINS margins = {-1, -1, -1, -1};
        DwmExtendFrameIntoClientArea(hWnd, &margins);
    }
#endif
}

void AtlasWindow::registerGlobalHotkey() {
#ifdef Q_OS_WIN
    HWND hWnd = reinterpret_cast<HWND>(winId());
    if (hWnd) {
        // Register Alt + Space as global hotkey (ID = 101)
        m_hotkeyRegistered = RegisterHotKey(hWnd, 101, MOD_ALT | MOD_NOREPEAT, VK_SPACE);
        if (m_hotkeyRegistered) {
            qDebug() << "[AtlasWindow] Global hotkey Alt+Space registered successfully.";
        } else {
            qWarning() << "[AtlasWindow] Failed to register global hotkey Alt+Space.";
        }
    }
#endif
}

void AtlasWindow::unregisterGlobalHotkey() {
#ifdef Q_OS_WIN
    if (m_hotkeyRegistered) {
        HWND hWnd = reinterpret_cast<HWND>(winId());
        if (hWnd) {
            UnregisterHotKey(hWnd, 101);
            m_hotkeyRegistered = false;
        }
    }
#endif
}

void AtlasWindow::toggleVisibility() {
    if (isVisible() && isActive()) {
        hide();
    } else {
        show();
        requestActivate();
    }
}

bool AtlasWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result) {
#ifdef Q_OS_WIN
    MSG *msg = static_cast<MSG *>(message);
    if (msg->message == WM_HOTKEY && msg->wParam == 101) {
        toggleVisibility();
        if (result) *result = 0;
        return true;
    }
#endif
    return QQuickWindow::nativeEvent(eventType, message, result);
}

void AtlasWindow::focusOutEvent(QFocusEvent *ev) {
    QQuickWindow::focusOutEvent(ev);
    // Auto-hide when launcher loses focus
    hide();
}
