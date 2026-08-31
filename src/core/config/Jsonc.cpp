#include "Jsonc.hpp"

namespace jsonc {

QByteArray stripComments(const QByteArray &input) {
    QByteArray out;
    out.reserve(input.size());

    bool inString = false;
    bool inLineComment = false;
    bool inBlockComment = false;

    for (qsizetype i = 0; i < input.size(); ++i) {
        const char c = input[i];
        const char next = (i + 1 < input.size()) ? input[i + 1] : '\0';

        if (inLineComment) {
            if (c == '\n') {
                inLineComment = false;
                out.append(c); // keep newlines so line numbers stay useful in errors
            }
            continue;
        }
        if (inBlockComment) {
            if (c == '*' && next == '/') {
                inBlockComment = false;
                ++i;
            }
            continue;
        }
        if (inString) {
            out.append(c);
            if (c == '\\' && i + 1 < input.size()) {
                out.append(next); // preserve the escaped character verbatim
                ++i;
            } else if (c == '"') {
                inString = false;
            }
            continue;
        }

        if (c == '"') {
            inString = true;
            out.append(c);
        } else if (c == '/' && next == '/') {
            inLineComment = true;
            ++i;
        } else if (c == '/' && next == '*') {
            inBlockComment = true;
            ++i;
        } else {
            out.append(c);
        }
    }

    return out;
}

} // namespace jsonc
