#pragma once

// Atlas fuzzy matcher.
//
// Two separate numbers per match, used for different jobs:
//  - `score`   (0-100, field-weighted): how good the match is -> use for RANKING.
//  - `quality` (0-100, weight-free, worst query word): how believable the match
//    is -> compare against MIN_QUALITY to FILTER, independent of query length.
// A subsequence that technically matches but no human would recognize (letters
// scattered mid-word across many runs) is rejected as incoherent.

#include <QString>
#include <QStringView>
#include <span>
#include <vector>

namespace fuzzy {

// Matches scoring below this quality should not be shown at all.
inline constexpr int MIN_QUALITY = 55;

// A parsed query: lowercased, split on whitespace. Reusable across candidates.
class Query {
public:
    explicit Query(const QString &text);

    bool isEmpty() const { return m_words.empty(); }
    const std::vector<QString> &words() const { return m_words; }

private:
    std::vector<QString> m_words;
};

struct WordMatch {
    bool matched = false;
    int quality = 0; // 0-100, normalized against a perfect match of this word
};

// Match a single (already lowercased) query word against a candidate string.
// Case-insensitive subsequence match with boundary/camel/consecutive bonuses
// and gap penalties; incoherent alignments come back as matched = false.
WordMatch matchWord(QStringView candidate, QStringView word);

struct Field {
    QStringView text;
    float weight = 1.0f; // e.g. title 1.0, subtitle 0.5, keywords 0.6
};

struct QueryScore {
    int score = 0;   // ranking value, 0-100
    int quality = 0; // filtering value: the worst query word's best quality
    bool matched = false;
};

// Every query word must match at least one field, else the whole query fails.
QueryScore scoreFields(std::span<const Field> fields, const Query &query);

} // namespace fuzzy
