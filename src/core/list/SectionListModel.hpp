#pragma once

// Flattens N ordered SectionSources into one QAbstractListModel with section
// header rows, stable keyboard navigation, and selection that survives
// rebuilds (matched by itemId, falling back to the first selectable row).

#include "SectionSource.hpp"

#include <QAbstractListModel>
#include <vector>

class SectionListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedIndexChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(QString selectedTitle READ selectedTitle NOTIFY selectedIndexChanged)

public:
    enum Role {
        IsSectionRole = Qt::UserRole + 1,
        IsSelectableRole,
        SectionNameRole,
        ItemIdRole,
        TitleRole,
        SubtitleRole,
        IconRole,
        TypeRole,
        AliasRole,
        IsCalculatorRole,
        CalcQuestionRole,
        CalcQuestionUnitRole,
        CalcAnswerRole,
        CalcAnswerUnitRole,
    };

    explicit SectionListModel(QObject *parent = nullptr);

    // Sources are not owned; register in display order.
    void addSource(SectionSource *source);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int selectedIndex() const { return m_selectedIndex; }
    void setSelectedIndex(int index);
    QString selectedTitle() const;

    Q_INVOKABLE int nextSelectableIndex(int from, int direction) const;
    Q_INVOKABLE void activateSelected();
    Q_INVOKABLE void activateRow(int row);
    Q_INVOKABLE void setFilterText(const QString &query) { applyFilter(query); }

signals:
    void selectedIndexChanged();
    void countChanged();

protected:
    // Push the current query to all sources, then rebuild.
    void applyFilter(const QString &query);

    // Reflatten all sources. preserveSelection keeps the previously selected
    // itemId selected if it still exists (used when async results stream in
    // while the user may be navigating).
    void rebuild(bool preserveSelection);

private:
    struct FlatRow {
        bool isHeader = false;
        int sourceIndex = 0;
        int itemIndex = 0; // unused for headers
    };

    int firstSelectable() const;
    QVariantMap rowItem(const FlatRow &row) const;

    std::vector<SectionSource *> m_sources;
    std::vector<FlatRow> m_rows;
    int m_selectedIndex = -1;
};
