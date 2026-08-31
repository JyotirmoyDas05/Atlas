#pragma once

// One pluggable section of a sectioned list (SectionListModel flattens many of
// these into a single QML model). A source owns its data and its activation
// behavior; the model owns flattening, roles, and selection.

#include <QString>
#include <QVariantMap>
#include <functional>
#include <memory>

class ActionPanelState;

class SectionSource {
public:
    virtual ~SectionSource() = default;

    // Empty name -> items render without a section header row.
    virtual QString sectionName() const = 0;
    virtual int count() const = 0;

    // Item role values. Recognized keys: itemId, title, subtitle, icon, type,
    // alias, isCalculator, calcQuestion, calcQuestionUnit, calcAnswer,
    // calcAnswerUnit. itemId must be stable across rebuilds for selection
    // preservation to work.
    virtual QVariantMap item(int index) const = 0;

    // Primary action (Enter / click).
    virtual void activate(int index) = 0;

    // Full action panel for this item (Ctrl+K). Default: no source-specific
    // actions; the caller falls back to whatever generic actions it wants.
    // Out-of-line (SectionSource.cpp): the body constructs/destroys a
    // unique_ptr<ActionPanelState>, which needs the complete type -- keeping
    // it out of the header means callers don't have to pull that in too.
    virtual std::unique_ptr<ActionPanelState> actionsFor(int index) const;

    // New query text. Synchronous sources filter now; async sources kick off
    // work and call notifyChanged(true) when results land.
    virtual void setFilter(const QString &query) = 0;

    void setOnChanged(std::function<void(bool preserveSelection)> callback) {
        m_onChanged = std::move(callback);
    }

protected:
    void notifyChanged(bool preserveSelection = false) {
        if (m_onChanged)
            m_onChanged(preserveSelection);
    }

private:
    std::function<void(bool)> m_onChanged;
};
