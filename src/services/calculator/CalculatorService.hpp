#pragma once

#include <QObject>
#include <QVariantList>
#include <QString>
#include <memory>

class QJSEngine;

// Evaluates math expressions using QJSEngine
// Returns { ok, question, answer, answerUnit }
struct CalcResult {
    bool ok = false;
    QString question;
    QString answer;
    QString answerUnit;
    QString questionUnit;
};

class CalculatorService : public QObject {
    Q_OBJECT

public:
    explicit CalculatorService(QObject *parent = nullptr);
    ~CalculatorService() override;

    CalcResult evaluate(const QString &expression) const;
    bool looksLikeMath(const QString &text) const;

private:
    // Lazily created and reused: QJSEngine construction is a full JS VM spin-up
    // (~40ms in debug) and evaluate() runs on every keystroke that looks mathy.
    mutable std::unique_ptr<QJSEngine> m_engine;
};
