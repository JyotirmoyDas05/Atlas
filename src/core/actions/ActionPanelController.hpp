#pragma once

// QML-facing singleton (context property "actionPanel"). Wraps whichever
// ActionPanelState is current for the selected row of whatever list is on
// screen -- see refreshFrom().

#include "ActionPanelModel.hpp"

class SectionListModel;

class ActionPanelController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject *model READ modelObject CONSTANT)
    Q_PROPERTY(bool open READ isOpen WRITE setOpen NOTIFY openChanged)
    Q_PROPERTY(bool hasActions READ hasActions NOTIFY stateChanged)
    Q_PROPERTY(QString primaryActionTitle READ primaryActionTitle NOTIFY stateChanged)
    Q_PROPERTY(QVariantList primaryActionShortcutTokens READ primaryActionShortcutTokens NOTIFY stateChanged)

public:
    explicit ActionPanelController(QObject *parent = nullptr);

    QObject *modelObject() { return &m_model; }
    bool isOpen() const { return m_open; }
    void setOpen(bool value);
    bool hasActions() const { return m_model.rowCount() > 0; }
    QString primaryActionTitle() const;
    QVariantList primaryActionShortcutTokens() const;

    // Rebuild the panel from whatever list is currently active. Pass nullptr
    // to clear (e.g. transient views with no selectable rows).
    void refreshFrom(const SectionListModel *source);

    Q_INVOKABLE void toggle() { setOpen(!m_open); }
    Q_INVOKABLE void close() { setOpen(false); }
    Q_INVOKABLE void moveUp();
    Q_INVOKABLE void moveDown();
    Q_INVOKABLE void activateCurrent();
    Q_INVOKABLE bool tryShortcut(int key, int modifiers);

signals:
    void openChanged();
    void stateChanged();

private:
    ActionPanelModel m_model;
    bool m_open = false;
};
