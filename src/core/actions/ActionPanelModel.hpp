#pragma once

// Flattens an ActionPanelState into a QAbstractListModel for QML, mirroring
// SectionListModel's shape (section headers + rows) but for actions.

#include "ActionPanelState.hpp"

#include <QAbstractListModel>
#include <memory>

class ActionPanelModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedIndexChanged)

public:
    enum Role {
        IsSectionRole = Qt::UserRole + 1,
        SectionNameRole,
        TitleRole,
        IconRole,
        ShortcutTokensRole,
        IsDangerRole,
    };

    explicit ActionPanelModel(QObject *parent = nullptr);

    // Takes ownership; nullptr clears the panel.
    void setState(std::unique_ptr<ActionPanelState> state);
    const ActionPanelState *state() const { return m_state.get(); }

    int rowCount(const QModelIndex & = {}) const override { return static_cast<int>(m_rows.size()); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int selectedIndex() const { return m_selectedIndex; }
    void setSelectedIndex(int index);

    Q_INVOKABLE int nextSelectableIndex(int from, int direction) const;
    Q_INVOKABLE void activateSelected();
    Q_INVOKABLE void activateRow(int row);

    // Finds and executes the action bound to this key combination, if any is
    // currently visible. Returns true if something executed.
    Q_INVOKABLE bool tryShortcut(int key, int modifiers);

signals:
    void selectedIndexChanged();
    void countChanged();
    void actionExecuted(); // panel should close (unless the action opted out)

private:
    struct FlatRow {
        bool isHeader = false;
        int sectionIndex = 0;
        int actionIndex = 0;
    };

    void rebuild();
    Action *actionAt(int row) const;

    std::unique_ptr<ActionPanelState> m_state;
    std::vector<FlatRow> m_rows;
    int m_selectedIndex = -1;
};
