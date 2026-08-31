#include "Navigation.hpp"

Navigation::Navigation(QObject *parent) : QObject(parent) {}

void Navigation::push(ViewHost *view) {
    m_stack.emplace_back(view);
    emit viewPushed(view->qmlComponentUrl(), view->qmlProperties());
    emit depthChanged();
}

void Navigation::pop() {
    if (m_stack.empty())
        return;
    // The QML StackView item may still hold bindings into the host while it is
    // being torn down; release the C++ side only after the event loop turn.
    ViewHost *view = m_stack.back().release();
    m_stack.pop_back();
    view->deleteLater();
    emit viewPopped();
    emit depthChanged();
}

void Navigation::popToRoot() {
    while (!m_stack.empty())
        pop();
}

QString Navigation::currentTitle() const {
    const ViewHost *view = currentView();
    return view ? view->navigationTitle() : QString();
}

QString Navigation::searchPlaceholder() const {
    const ViewHost *view = currentView();
    return view ? view->searchPlaceholder() : QStringLiteral("Search apps, files, commands…");
}
