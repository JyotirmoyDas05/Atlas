#pragma once

// Base for pushed views that are just a single sectioned list (the common
// case). Owns the model, wires filter routing, and exposes it to QML.

#include "ViewHost.hpp"
#include "core/list/SectionListModel.hpp"

class ListViewHostBase : public ViewHost {
    Q_OBJECT
    Q_PROPERTY(QObject *model READ modelObject CONSTANT)

public:
    using ViewHost::ViewHost;

    QObject *modelObject() { return &m_model; }
    SectionListModel &model() { return m_model; }

    void setFilter(const QString &text) override { m_model.setFilterText(text); }

protected:
    SectionListModel m_model;
};
