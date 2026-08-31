#include "SectionListModel.hpp"

#include <algorithm>

SectionListModel::SectionListModel(QObject *parent) : QAbstractListModel(parent) {}

void SectionListModel::addSource(SectionSource *source) {
    m_sources.push_back(source);
    source->setOnChanged([this](bool preserveSelection) { rebuild(preserveSelection); });
}

int SectionListModel::rowCount(const QModelIndex &) const {
    return static_cast<int>(m_rows.size());
}

QVariantMap SectionListModel::rowItem(const FlatRow &row) const {
    return m_sources[row.sourceIndex]->item(row.itemIndex);
}

QVariant SectionListModel::data(const QModelIndex &index, int role) const {
    const int i = index.row();
    if (i < 0 || i >= static_cast<int>(m_rows.size()))
        return {};
    const FlatRow &row = m_rows[i];

    switch (role) {
    case IsSectionRole:
        return row.isHeader;
    case IsSelectableRole:
        return !row.isHeader;
    case SectionNameRole:
        return m_sources[row.sourceIndex]->sectionName();
    default:
        break;
    }

    if (row.isHeader)
        return role == TitleRole ? QVariant(m_sources[row.sourceIndex]->sectionName()) : QVariant();

    const QVariantMap item = rowItem(row);
    switch (role) {
    case ItemIdRole:          return item.value("itemId");
    case TitleRole:           return item.value("title");
    case SubtitleRole:        return item.value("subtitle");
    case IconRole:            return item.value("icon");
    case TypeRole:            return item.value("type");
    case AliasRole:           return item.value("alias");
    case IsCalculatorRole:    return item.value("isCalculator", false);
    case CalcQuestionRole:    return item.value("calcQuestion");
    case CalcQuestionUnitRole:return item.value("calcQuestionUnit");
    case CalcAnswerRole:      return item.value("calcAnswer");
    case CalcAnswerUnitRole:  return item.value("calcAnswerUnit");
    default:                  return {};
    }
}

QHash<int, QByteArray> SectionListModel::roleNames() const {
    return {
        {IsSectionRole, "isSection"},
        {IsSelectableRole, "isSelectable"},
        {SectionNameRole, "sectionName"},
        {ItemIdRole, "itemId"},
        {TitleRole, "title"},
        {SubtitleRole, "subtitle"},
        {IconRole, "icon"},
        {TypeRole, "type"},
        {AliasRole, "alias"},
        {IsCalculatorRole, "isCalculator"},
        {CalcQuestionRole, "calcQuestion"},
        {CalcQuestionUnitRole, "calcQuestionUnit"},
        {CalcAnswerRole, "calcAnswer"},
        {CalcAnswerUnitRole, "calcAnswerUnit"},
    };
}

void SectionListModel::setSelectedIndex(int index) {
    const int count = static_cast<int>(m_rows.size());
    if (count == 0)
        index = -1;
    else
        index = std::clamp(index, -1, count - 1);

    if (index == m_selectedIndex)
        return;
    m_selectedIndex = index;
    emit selectedIndexChanged();
}

QString SectionListModel::selectedTitle() const {
    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_rows.size()))
        return {};
    const FlatRow &row = m_rows[m_selectedIndex];
    if (row.isHeader)
        return {};
    return rowItem(row).value("title").toString();
}

int SectionListModel::nextSelectableIndex(int from, int direction) const {
    const int count = static_cast<int>(m_rows.size());
    if (count == 0)
        return -1;

    int next = from + direction;
    while (next >= 0 && next < count) {
        if (!m_rows[next].isHeader)
            return next;
        next += direction;
    }
    return from;
}

int SectionListModel::firstSelectable() const {
    return nextSelectableIndex(-1, 1);
}

void SectionListModel::activateSelected() {
    activateRow(m_selectedIndex);
}

void SectionListModel::activateRow(int row) {
    if (row < 0 || row >= static_cast<int>(m_rows.size()))
        return;
    const FlatRow &flat = m_rows[row];
    if (flat.isHeader)
        return;
    m_sources[flat.sourceIndex]->activate(flat.itemIndex);
}

void SectionListModel::applyFilter(const QString &query) {
    for (SectionSource *source : m_sources)
        source->setFilter(query);
    rebuild(/*preserveSelection=*/false);
}

void SectionListModel::rebuild(bool preserveSelection) {
    // Snapshot the selected item's identity before the flatten.
    QVariant selectedId;
    const bool selectionWasDefault =
        m_selectedIndex < 0 || m_selectedIndex == firstSelectable();
    if (preserveSelection && m_selectedIndex >= 0 &&
        m_selectedIndex < static_cast<int>(m_rows.size()) &&
        !m_rows[m_selectedIndex].isHeader) {
        selectedId = rowItem(m_rows[m_selectedIndex]).value("itemId");
    }

    beginResetModel();
    m_rows.clear();
    for (int s = 0; s < static_cast<int>(m_sources.size()); ++s) {
        const SectionSource *source = m_sources[s];
        const int n = source->count();
        if (n == 0)
            continue;
        if (!source->sectionName().isEmpty())
            m_rows.push_back({.isHeader = true, .sourceIndex = s});
        for (int i = 0; i < n; ++i)
            m_rows.push_back({.isHeader = false, .sourceIndex = s, .itemIndex = i});
    }
    endResetModel();
    emit countChanged();

    int newSelection = firstSelectable();
    if (preserveSelection && !selectionWasDefault && selectedId.isValid()) {
        for (int i = 0; i < static_cast<int>(m_rows.size()); ++i) {
            if (!m_rows[i].isHeader && rowItem(m_rows[i]).value("itemId") == selectedId) {
                newSelection = i;
                break;
            }
        }
    }

    // Force the notify even if the numeric index is unchanged: the row content
    // under that index may differ after a rebuild.
    m_selectedIndex = newSelection;
    emit selectedIndexChanged();
}
