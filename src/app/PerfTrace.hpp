#pragma once

#include <QString>

// Lightweight perf harness. Every measurement lands as one "[Perf] name ..."
// line in the debug log, so latency targets (toggle <8ms, search, index build)
// are tracked from real runs instead of guessed.
//
// Thread-safe; callable from any thread including the low-level hook thread.
namespace PerfTrace {

// Log a one-off event line: "[Perf] <message>".
void log(const QString &message);

// Record the moment the global hotkey was pressed (called on the hook thread).
void hotkeyPressed();

// Microseconds since the last hotkeyPressed(), or -1 if none/consumed.
// Consumes the timestamp so a toggle is only measured once.
qint64 takeHotkeyElapsedUs();

} // namespace PerfTrace
