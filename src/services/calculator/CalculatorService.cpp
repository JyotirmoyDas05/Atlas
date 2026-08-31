#include "CalculatorService.hpp"
#include <QJSEngine>
#include <QRegularExpression>
#include <cmath>

// Unit conversion table: "from unit" -> { factor to base, "base unit" }
static const QHash<QString, std::pair<double, QString>> UNIT_TABLE = {
    // Length
    {"km",    {1000.0,       "m"}},
    {"m",     {1.0,          "m"}},
    {"cm",    {0.01,         "m"}},
    {"mm",    {0.001,        "m"}},
    {"mi",    {1609.344,     "m"}},
    {"miles", {1609.344,     "m"}},
    {"ft",    {0.3048,       "m"}},
    {"feet",  {0.3048,       "m"}},
    {"in",    {0.0254,       "m"}},
    {"inch",  {0.0254,       "m"}},
    // Weight
    {"kg",    {1.0,          "kg"}},
    {"g",     {0.001,        "kg"}},
    {"lb",    {0.453592,     "kg"}},
    {"lbs",   {0.453592,     "kg"}},
    {"oz",    {0.0283495,    "kg"}},
    // Temperature handled separately
};

static std::optional<CalcResult> tryConversion(const QString &expr) {
    // Matches: "100 km to miles", "5 ft to m", "100 to km" patterns
    static const QRegularExpression re(
        R"(^\s*(\d+(?:\.\d+)?)\s*([a-zA-Z]+)\s+(?:to|in|=)\s+([a-zA-Z]+)\s*$)",
        QRegularExpression::CaseInsensitiveOption);

    auto m = re.match(expr.trimmed());
    if (!m.hasMatch()) return std::nullopt;

    const double value   = m.captured(1).toDouble();
    const QString fromU  = m.captured(2).toLower();
    const QString toU    = m.captured(3).toLower();

    // Temperature special case
    if ((fromU == "c" || fromU == "celsius") && (toU == "f" || toU == "fahrenheit")) {
        const double r = value * 9.0 / 5.0 + 32.0;
        return CalcResult{true, m.captured(0).trimmed(), QString::number(r, 'f', 2), "°F", "°C"};
    }
    if ((fromU == "f" || fromU == "fahrenheit") && (toU == "c" || toU == "celsius")) {
        const double r = (value - 32.0) * 5.0 / 9.0;
        return CalcResult{true, m.captured(0).trimmed(), QString::number(r, 'f', 2), "°C", "°F"};
    }

    auto it1 = UNIT_TABLE.find(fromU);
    auto it2 = UNIT_TABLE.find(toU);
    if (it1 == UNIT_TABLE.end() || it2 == UNIT_TABLE.end()) return std::nullopt;

    const auto &[f1, base1] = it1.value();
    const auto &[f2, base2] = it2.value();
    if (base1 != base2) return std::nullopt;

    const double result = value * f1 / f2;
    return CalcResult{true, m.captured(0).trimmed(),
                      QString::number(result, 'g', 8), toU, fromU};
}

CalculatorService::CalculatorService(QObject *parent)
    : QObject(parent) {}

CalculatorService::~CalculatorService() = default;

bool CalculatorService::looksLikeMath(const QString &text) const {
    // Must contain at least one digit and an operator, or a conversion pattern.
    // The digit requirement is load-bearing: without it, any hyphenated text
    // query ("atlas-indexer") triggered a JS evaluation on every keystroke.
    static const QRegularExpression digitRe(R"(\d)");
    static const QRegularExpression opRe(R"([\+\-\*\/\^])");
    static const QRegularExpression convRe(R"(\d+\s*[a-zA-Z]+\s+(to|in)\s+[a-zA-Z]+)",
                                           QRegularExpression::CaseInsensitiveOption);
    return convRe.match(text).hasMatch() ||
           (digitRe.match(text).hasMatch() && opRe.match(text).hasMatch());
}

CalcResult CalculatorService::evaluate(const QString &expression) const {
    if (expression.trimmed().isEmpty()) return {};

    // Try unit conversion first
    if (auto conv = tryConversion(expression)) return *conv;

    // Sanitize: only allow math-safe characters
    const QString safe = expression.trimmed()
        .replace('^', "**")           // JS exponentiation
        .replace("π", "Math.PI")
        .replace("pi", "Math.PI", Qt::CaseInsensitive)
        .replace("sqrt(", "Math.sqrt(", Qt::CaseInsensitive)
        .replace("sin(", "Math.sin(", Qt::CaseInsensitive)
        .replace("cos(", "Math.cos(", Qt::CaseInsensitive)
        .replace("tan(", "Math.tan(", Qt::CaseInsensitive)
        .replace("log(", "Math.log10(", Qt::CaseInsensitive)
        .replace("ln(", "Math.log(", Qt::CaseInsensitive)
        .replace("abs(", "Math.abs(", Qt::CaseInsensitive);

    static const QRegularExpression validChars(R"([^0-9\+\-\*\/\.\(\)\s,MathsqrcoinablegPIlogABS_])");
    if (validChars.match(safe).hasMatch()) return {};

    if (!m_engine) {
        m_engine = std::make_unique<QJSEngine>();
    }
    QJSValue result = m_engine->evaluate(safe);

    if (result.isError() || result.isUndefined()) return {};

    const double val = result.toNumber();
    if (std::isnan(val) || std::isinf(val)) return {};

    QString answerStr;
    if (val == std::floor(val) && std::abs(val) < 1e15)
        answerStr = QString::number(static_cast<long long>(val));
    else
        answerStr = QString::number(val, 'g', 10);

    return CalcResult{true, expression.trimmed(), answerStr, "", ""};
}
