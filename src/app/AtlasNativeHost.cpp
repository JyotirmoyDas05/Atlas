#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "AtlasNativeHost.hpp"
#include "PerfTrace.hpp"
#include <QDebug>
#include <QGuiApplication>
#include <QScreen>
#include <QEvent>
#include <QMetaObject>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>

namespace {

constexpr UINT_PTR CHROME_SUBCLASS_ID = 101;
AtlasNativeHost *g_nativeHost = nullptr;

LRESULT CALLBACK chromeSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                    UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    switch (msg) {
    case WM_NCCALCSIZE:
        // Strip native window frame / white background titlebar
        if (wParam == TRUE) return 0;
        break;
    case WM_WINDOWPOSCHANGING: {
        auto *wp = reinterpret_cast<WINDOWPOS *>(lParam);
        if (wp && !(wp->flags & SWP_NOSIZE)) wp->flags |= SWP_NOCOPYBITS;
        break;
    }
    case WM_NCACTIVATE:
        return DefWindowProcW(hwnd, msg, wParam, -1);
    case WM_SHOWWINDOW:
        if (wParam == TRUE) {
            if (g_nativeHost) g_nativeHost->applyWindowsChrome();
            DefWindowProcW(hwnd, WM_NCACTIVATE, TRUE, -1);
        }
        break;
    case WM_NCHITTEST: {
        LRESULT hit = DefSubclassProc(hwnd, msg, wParam, lParam);
        return (hit >= HTLEFT && hit <= HTBOTTOMRIGHT) ? HTCLIENT : hit;
    }
    default:
        break;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

// Check if Alt modifier is currently down
bool isAltDown() {
    return (GetAsyncKeyState(VK_MENU) < 0);
}

// Low-level keyboard hook callback (matches Vicinae's hook mechanism)
LRESULT CALLBACK keyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        const auto *k = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
        const bool down = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);

        // Check for Alt + Space (VK_SPACE = 0x20)
        if (k->vkCode == VK_SPACE && isAltDown()) {
            if (down && g_nativeHost) {
                PerfTrace::hotkeyPressed();
                QMetaObject::invokeMethod(g_nativeHost, &AtlasNativeHost::toggleVisibility, Qt::QueuedConnection);
            }
            return 1; // Eat key event so Windows doesn't trigger system menu
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

} // namespace
#endif

AtlasNativeHost::AtlasNativeHost(QQuickWindow *window, QObject *parent)
    : QObject(parent), m_window(window) {
    if (m_window) {
        g_nativeHost = this;
        m_window->installEventFilter(this);

        // Make QQuickWindow transparent so DWM rounded corners show with no white background
        m_window->setColor(QColor(Qt::transparent));

        // Center window on primary screen
        if (QScreen *primary = QGuiApplication::primaryScreen()) {
            QRect sg = primary->geometry();
            int w = 760;
            int h = 460;
            int x = sg.x() + (sg.width() - w) / 2;
            int y = sg.y() + (sg.height() - h) / 3;
            m_window->setGeometry(x, y, w, h);
        }

        applyWindowsChrome();
        startKeyboardHook();

        // First-ever window show pays QML layout + swapchain creation
        // (~130ms measured). Render one invisible frame at startup so the
        // first real toggle is as fast as every other one.
        QTimer::singleShot(50, this, &AtlasNativeHost::prewarmRender);
    }
}

void AtlasNativeHost::prewarmRender() {
    if (!m_window || m_window->isVisible()) return;

    m_prewarmPending = true;
    m_window->setOpacity(0.0);

    auto *conn = new QMetaObject::Connection;
    *conn = connect(m_window, &QQuickWindow::frameSwapped, this, [this, conn]() {
        QObject::disconnect(*conn);
        delete conn;
        // Only clean up if a real showWindow() didn't interleave with us.
        if (m_prewarmPending) {
            m_prewarmPending = false;
            m_window->hide();
            m_window->setOpacity(1.0);
            PerfTrace::log(QStringLiteral("prewarm_render done"));
        }
    });

    m_window->show();
}

AtlasNativeHost::~AtlasNativeHost() {
    stopKeyboardHook();
    g_nativeHost = nullptr;
}

void AtlasNativeHost::applyWindowsChrome() {
#ifdef Q_OS_WIN
    if (!m_window) return;

    HWND hWnd = reinterpret_cast<HWND>(m_window->winId());
    if (!hWnd) return;

    // Hide window from Windows Taskbar and Alt+Tab switcher (Raycast style)
    LONG_PTR exStyle = GetWindowLongPtrW(hWnd, GWL_EXSTYLE);
    exStyle |= WS_EX_TOOLWINDOW;
    exStyle &= ~WS_EX_APPWINDOW;
    SetWindowLongPtrW(hWnd, GWL_EXSTYLE, exStyle);

    // Subclass window procedure to eliminate white non-client frame (WM_NCCALCSIZE = 0)
    LONG_PTR style = GetWindowLongPtrW(hWnd, GWL_STYLE);
    if (!(style & WS_THICKFRAME)) {
        SetWindowLongPtrW(hWnd, GWL_STYLE, style | WS_THICKFRAME);
    }
    SetWindowSubclass(hWnd, chromeSubclassProc, CHROME_SUBCLASS_ID, 0);
    SetWindowPos(hWnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

    // Apply native DWM rounded corners
    DWM_WINDOW_CORNER_PREFERENCE corner = DWMWCP_ROUND;
    DwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));

    // Fluent UI Mica backdrop blur (DWMSBT_MAINWINDOW)
    DWM_SYSTEMBACKDROP_TYPE backdrop = DWMSBT_MAINWINDOW;
    DwmSetWindowAttribute(hWnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop, sizeof(backdrop));

    // Transparent DWM frame extension
    MARGINS margins = {-1, -1, -1, -1};
    DwmExtendFrameIntoClientArea(hWnd, &margins);

    // Immersive Dark Mode
    BOOL dark = TRUE;
    DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
#endif
}

void AtlasNativeHost::startKeyboardHook() {
#ifdef Q_OS_WIN
    if (m_hookInstalled) return;

    m_hookThread = std::thread([this]() {
        constexpr int FALLBACK_HOTKEY_ID = 0xA71A;

        m_hookThreadId = GetCurrentThreadId();
        HHOOK hook = SetWindowsHookExW(WH_KEYBOARD_LL, keyboardHookProc, GetModuleHandleW(nullptr), 0);
        m_hookInstalled = (hook != nullptr);

        // Safety net: Windows silently uninstalls a low-level hook whose
        // callback ever exceeds the system timeout. While the hook is alive it
        // swallows Alt+Space, so this hotkey never fires; if WM_HOTKEY arrives,
        // the hook is dead — toggle anyway and reinstall it.
        const bool fallbackRegistered =
            RegisterHotKey(nullptr, FALLBACK_HOTKEY_ID, MOD_ALT | MOD_NOREPEAT, VK_SPACE);

        if (!hook && !fallbackRegistered) {
            qWarning() << "[AtlasNativeHost] Failed to install keyboard hook AND fallback hotkey.";
            return;
        }
        qDebug() << "[AtlasNativeHost] Hotkey active (Alt + Space): hook="
                 << (hook != nullptr) << "fallback=" << fallbackRegistered;

        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
            if (msg.message == WM_HOTKEY && msg.wParam == FALLBACK_HOTKEY_ID) {
                PerfTrace::hotkeyPressed();
                if (g_nativeHost) {
                    QMetaObject::invokeMethod(g_nativeHost, &AtlasNativeHost::toggleVisibility,
                                              Qt::QueuedConnection);
                }
                if (hook) UnhookWindowsHookEx(hook);
                hook = SetWindowsHookExW(WH_KEYBOARD_LL, keyboardHookProc,
                                         GetModuleHandleW(nullptr), 0);
                m_hookInstalled = (hook != nullptr);
                qWarning() << "[AtlasNativeHost] Hook was dead (fallback hotkey fired); reinstalled ="
                           << m_hookInstalled.load();
                continue;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (fallbackRegistered) UnregisterHotKey(nullptr, FALLBACK_HOTKEY_ID);
        if (hook) UnhookWindowsHookEx(hook);
    });
#endif
}

void AtlasNativeHost::stopKeyboardHook() {
#ifdef Q_OS_WIN
    if (m_hookThread.joinable()) {
        PostThreadMessageW(m_hookThreadId, WM_QUIT, 0, 0);
        m_hookThread.join();
        m_hookInstalled = false;
    }
#endif
}

#ifdef Q_OS_WIN
static void forceForegroundFocusWin(HWND hWnd) {
    if (!hWnd) return;
    DWORD currentThreadId = GetCurrentThreadId();
    DWORD foregroundThreadId = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
    if (currentThreadId != foregroundThreadId && foregroundThreadId != 0) {
        AttachThreadInput(currentThreadId, foregroundThreadId, TRUE);
        SetForegroundWindow(hWnd);
        BringWindowToTop(hWnd);
        SetFocus(hWnd);
        AttachThreadInput(currentThreadId, foregroundThreadId, FALSE);
    } else {
        SetForegroundWindow(hWnd);
        BringWindowToTop(hWnd);
        SetFocus(hWnd);
    }
}
#endif

void AtlasNativeHost::showWindow() {
    if (!m_window) return;

    // If a prewarm is mid-flight, take over: make the window real.
    if (m_prewarmPending) {
        m_prewarmPending = false;
        m_window->setOpacity(1.0);
    }

    // Measure hotkey -> first rendered frame (the real perceived toggle latency).
    auto *conn = new QMetaObject::Connection;
    *conn = connect(m_window, &QQuickWindow::frameSwapped, this, [conn]() {
        const qint64 us = PerfTrace::takeHotkeyElapsedUs();
        if (us >= 0)
            PerfTrace::log(QStringLiteral("toggle_show hotkey_to_frame_ms=%1")
                               .arg(us / 1000.0, 0, 'f', 2));
        QObject::disconnect(*conn);
        delete conn;
    });

    applyWindowsChrome();

    m_window->show();
    m_window->raise();
    m_window->requestActivate();

#ifdef Q_OS_WIN
    HWND hWnd = reinterpret_cast<HWND>(m_window->winId());
    if (hWnd) {
        applyWindowsChrome();
        forceForegroundFocusWin(hWnd);
    }
#endif
}

void AtlasNativeHost::hideWindow() {
    if (!m_window) return;
    m_window->hide();
}

void AtlasNativeHost::toggleVisibility() {
    if (!m_window) return;

    if (m_window->isVisible() && m_window->isActive()) {
        hideWindow();
    } else {
        showWindow();
    }
}

bool AtlasNativeHost::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) {
    Q_UNUSED(eventType);
    Q_UNUSED(message);
    Q_UNUSED(result);
    return false;
}

bool AtlasNativeHost::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_window && event->type() == QEvent::WindowDeactivate) {
        // Hide window when clicking outside or focus lost
        hideWindow();
    }
    return QObject::eventFilter(watched, event);
}
