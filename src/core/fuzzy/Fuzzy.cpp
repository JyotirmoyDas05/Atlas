#include "Fuzzy.hpp"

#include <QStringList>
#include <algorithm>

namespace fuzzy {

namespace {

constexpr int CHAR_SCORE = 4;        // every matched character
constexpr int CONSECUTIVE_BONUS = 8; // each char extending a run
constexpr int BOUNDARY_BONUS = 16;   // match starts a word / camel hump
constexpr int PREFIX_BONUS = 8;      // extra on top when match is at index 0
constexpr int GAP_PENALTY = 1;       // per skipped candidate char, capped
constexpr int GAP_PENALTY_CAP = 20;

bool isSeparator(QChar c) {
    return c.isSpace() || c == u'-' || c == u'_' || c == u'.' || c == u'/' ||
           c == u'\\' || c == u'(' || c == u'[' || c == u':';
}

// A boundary is the start of something a human reads as a word.
bool isBoundary(QStringView s, qsizetype i) {
    if (i == 0)
        return true;
    const QChar prev = s[i - 1];
    const QChar cur = s[i];
    if (isSeparator(prev))
        return true;
    if (prev.isLower() && cur.isUpper()) // camelCase hump
        return true;
    if (!prev.isDigit() && cur.isDigit()) // letter -> digit transition
        return true;
    return false;
}

int perfectScore(qsizetype wordLen) {
    // First char at a boundary at index 0, rest one consecutive run.
    return BOUNDARY_BONUS + PREFIX_BONUS + CHAR_SCORE +
           static_cast<int>(wordLen - 1) * (CHAR_SCORE + CONSECUTIVE_BONUS);
}

} // namespace

Query::Query(const QString &text) {
    for (const auto &part : text.split(u' ', Qt::SkipEmptyParts)) {
        m_words.push_back(part.toLower());
    }
}

WordMatch matchWord(QStringView candidate, QStringView word) {
    if (word.isEmpty() || candidate.isEmpty())
        return {};

    // Greedy forward subsequence walk. Not an optimal-alignment search, but
    // combined with the boundary bonuses and coherence rejection it produces
    // human-plausible results at a fraction of the cost.
    int raw = 0;
    int gaps = 0;
    int runs = 0;             // count of separate consecutive-match runs
    bool inRun = false;
    bool firstAtBoundary = false;
    qsizetype wi = 0;
    qsizetype firstMatch = -1;

    for (qsizetype ci = 0; ci < candidate.size() && wi < word.size(); ++ci) {
        if (candidate[ci].toLower() != word[wi]) {
            if (firstMatch >= 0)
                ++gaps;
            inRun = false;
            continue;
        }

        raw += CHAR_SCORE;
        if (inRun) {
            raw += CONSECUTIVE_BONUS;
        } else {
            ++runs;
            if (isBoundary(candidate, ci)) {
                raw += BOUNDARY_BONUS;
                if (ci == 0)
                    raw += PREFIX_BONUS;
            }
        }
        if (firstMatch < 0) {
            firstMatch = ci;
            firstAtBoundary = isBoundary(candidate, ci);
        }
        inRun = true;
        ++wi;
    }

    if (wi < word.size())
        return {}; // not a subsequence

    // Coherence: an alignment scattered over many runs that doesn't even start
    // at a word boundary is noise, not a match a human intended.
    const int maxRuns = std::max<qsizetype>(2, word.size() / 2 + 1);
    if (!firstAtBoundary && runs > maxRuns)
        return {};

    raw -= std::min(gaps * GAP_PENALTY, GAP_PENALTY_CAP);
    raw = std::max(raw, 1);

    const int quality =
        std::clamp(raw * 100 / perfectScore(word.size()), 1, 100);
    return {.matched = true, .quality = quality};
}

QueryScore scoreFields(std::span<const Field> fields, const Query &query) {
    if (query.isEmpty() || fields.empty())
        return {};

    int total = 0;
    int worstQuality = 100;

    for (const auto &word : query.words()) {
        int bestWeighted = -1;
        int bestQuality = -1;
        for (const auto &field : fields) {
            const WordMatch m = matchWord(field.text, word);
            if (!m.matched)
                continue;
            const int weighted = static_cast<int>(m.quality * field.weight);
            if (weighted > bestWeighted)
                bestWeighted = weighted;
            if (m.quality > bestQuality)
                bestQuality = m.quality;
        }
        if (bestWeighted < 0)
            return {}; // every query word must land somewhere
        total += bestWeighted;
        worstQuality = std::min(worstQuality, bestQuality);
    }

    return {
        .score = total / static_cast<int>(query.words().size()),
        .quality = worstQuality,
        .matched = true,
    };
}

} // namespace fuzzy
