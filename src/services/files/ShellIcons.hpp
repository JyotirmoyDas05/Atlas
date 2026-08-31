#pragma once

#include <QString>

namespace ShellIcons {

// Returns a file:/// URL to a cached PNG of the native shell icon for the
// given path (by extension, or the folder icon), or an empty string.
QString iconUrlFor(const QString &filePath, bool isDir);

} // namespace ShellIcons
