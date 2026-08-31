#pragma once

#include <QByteArray>

namespace jsonc {

// Strips // line comments and block comments from JSON-with-comments text
// so the result can be fed to QJsonDocument::fromJson. Comment markers
// inside string literals (respecting \" escapes) are left untouched.
QByteArray stripComments(const QByteArray &input);

} // namespace jsonc
