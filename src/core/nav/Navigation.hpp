#pragma once

// Logical view stack. Depth 0 is the root search screen (owned by the window,
// not a ViewHost); pushed views stack above it. QML mirrors this with a
// StackView driven purely by the viewPushed/viewPopped signals.

#include "ViewHost.hpp"

#include <memory>
#include <vector>

class Navigation : public QObject {
    Q_OBJECT
    Q_PROPERTY(int depth READ depth NOTIFY depthChanged)
    Q_PROPERTY(QObject *currentView READ currentViewObject NOTIFY depthChanged)
    Q_PROPERTY(QString currentTitle READ currentTitle NOTIFY depthChanged)
    Q_PROPERTY(QString searchPlaceholder READ searchPlaceholder NOTIFY depthChanged)

public:
    explicit Navigation(QObject *parent = nullptr);

    // Takes ownership.
    void push(ViewHost *view);
    Q_INVOKABLE void pop();
    Q_INVOKABLE void popToRoot();

    int depth() const { return static_cast<int>(m_stack.size()); }
    ViewHost *currentView() const { return m_stack.empty() ? nullptr : m_stack.back().get(); }
    QObject *currentViewObject() const { return currentView(); }
    QString currentTitle() const;
    QString searchPlaceholder() const;

signals:
    void viewPushed(const QUrl &componentUrl, const QVariantMap &properties);
    void viewPopped();
    void depthChanged();

private:
    std::vector<std::unique_ptr<ViewHost>> m_stack;
};
