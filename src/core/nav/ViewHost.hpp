#pragma once

// A pushable view: C++ host object + the QML component that renders it.
// The QML side receives the host via the `viewHost` property and duck-types
// against whatever Q_PROPERTYs the concrete host exposes.

#include <QObject>
#include <QUrl>
#include <QVariantMap>

class ViewHost : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString navigationTitle READ navigationTitle CONSTANT)
    Q_PROPERTY(QString searchPlaceholder READ searchPlaceholder CONSTANT)

public:
    using QObject::QObject;

    virtual QUrl qmlComponentUrl() const = 0;
    virtual QVariantMap qmlProperties() {
        return {{QStringLiteral("viewHost"), QVariant::fromValue<QObject *>(this)}};
    }

    virtual QString navigationTitle() const { return {}; }
    virtual QString searchPlaceholder() const { return QStringLiteral("Search…"); }

    // The window's search input routes here while this view is on top.
    virtual void setFilter(const QString &text) { Q_UNUSED(text); }
    Q_INVOKABLE void setFilterText(const QString &text) { setFilter(text); }
};
