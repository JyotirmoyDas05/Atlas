#pragma once

// A registrable command: builtins today, extension-provided commands later.
// Both will be the same thing to root search once extensions exist.

#include <QString>
#include <QStringList>
#include <functional>

class Navigation;

class Command {
public:
    Command(QString id, QString title, std::function<void(Navigation &)> execute)
        : m_id(std::move(id)), m_title(std::move(title)), m_execute(std::move(execute)) {}

    // Stable within its registry; used as the root-search itemId and, later,
    // as the key for aliases/hotkeys/favorites in config.
    QString id() const { return m_id; }
    QString title() const { return m_title; }

    QString icon() const { return m_icon; }
    void setIcon(QString icon) { m_icon = std::move(icon); }

    QStringList keywords() const { return m_keywords; }
    void setKeywords(QStringList keywords) { m_keywords = std::move(keywords); }

    void execute(Navigation &nav) const {
        if (m_execute)
            m_execute(nav);
    }

private:
    QString m_id;
    QString m_title;
    QString m_icon;
    QStringList m_keywords;
    std::function<void(Navigation &)> m_execute;
};
