#include "PerfTrace.hpp"

#include <QDebug>
#include <QElapsedTimer>

#include <atomic>

namespace PerfTrace {

namespace {

// Monotonic clock shared by all threads.
QElapsedTimer &clock() {
    static QElapsedTimer timer = [] {
        QElapsedTimer t;
        t.start();
        return t;
    }();
    return timer;
}

std::atomic<qint64> g_hotkeyNs{-1};

} // namespace

void log(const QString &message) {
    qInfo().noquote() << "[Perf]" << message;
}

void hotkeyPressed() {
    g_hotkeyNs.store(clock().nsecsElapsed(), std::memory_order_relaxed);
}

qint64 takeHotkeyElapsedUs() {
    const qint64 pressed = g_hotkeyNs.exchange(-1, std::memory_order_relaxed);
    if (pressed < 0)
        return -1;
    return (clock().nsecsElapsed() - pressed) / 1000;
}

} // namespace PerfTrace
