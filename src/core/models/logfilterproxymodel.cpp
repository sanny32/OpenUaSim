// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file logfilterproxymodel.cpp
/// \brief Implements the level, category and text filter over LogModel.
///

#include "logfilterproxymodel.h"
#include "logmodel.h"

///
/// \brief Constructs a filter showing every entry.
/// \param parent Owning QObject.
///
LogFilterProxyModel::LogFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setFilterKeyColumn(-1);
}

///
/// \brief Shows or hides entries of a level.
/// \param level Level to toggle.
/// \param visible True to show the level.
///
void LogFilterProxyModel::setLevelVisible(LogEntry::Level level, bool visible)
{
    if (isLevelVisible(level) == visible)
        return;
    beginFilterChange();
    if (visible)
        _hiddenLevels.remove(level);
    else
        _hiddenLevels.insert(level);
    finishFilterChange();
}

///
/// \brief Shows or hides entries of a category.
/// \param category Category to toggle.
/// \param visible True to show the category.
///
void LogFilterProxyModel::setCategoryVisible(LogEntry::Category category, bool visible)
{
    if (isCategoryVisible(category) == visible)
        return;
    beginFilterChange();
    if (visible)
        _hiddenCategories.remove(category);
    else
        _hiddenCategories.insert(category);
    finishFilterChange();
}

///
/// \brief Reports whether entries of a level are shown.
/// \param level Level to check.
/// \return True when the level is visible.
///
bool LogFilterProxyModel::isLevelVisible(LogEntry::Level level) const
{
    return !_hiddenLevels.contains(level);
}

///
/// \brief Reports whether entries of a category are shown.
/// \param category Category to check.
/// \return True when the category is visible.
///
bool LogFilterProxyModel::isCategoryVisible(LogEntry::Category category) const
{
    return !_hiddenCategories.contains(category);
}

///
/// \brief Returns the log entry behind a proxy row.
/// \param index Proxy index; any column of the row.
/// \return Pointer to the entry, or nullptr when the index is invalid
///         or the source is not a LogModel.
///
const LogEntry *LogFilterProxyModel::entry(const QModelIndex &index) const
{
    const LogModel *model = logModel();
    const QModelIndex source = mapToSource(index);
    if (!model || !source.isValid())
        return nullptr;
    return &model->item(source.row());
}

///
/// \brief Accepts a row when its level and category are visible and its text matches the search.
/// \param sourceRow Row in the source model.
/// \param sourceParent Parent in the source model.
/// \return True to show the row.
///
bool LogFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (!QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent))
        return false;
    const LogModel *model = logModel();
    if (!model)
        return true;
    const LogEntry &entry = model->item(sourceRow);
    return isLevelVisible(entry.level) && isCategoryVisible(entry.category);
}

///
/// \brief Returns the source model as a LogModel.
/// \return Source model, or nullptr when it is of another type.
///
const LogModel *LogFilterProxyModel::logModel() const
{
    return qobject_cast<const LogModel *>(sourceModel());
}

///
/// \brief Re-filters the rows after a change announced with beginFilterChange().
///
void LogFilterProxyModel::finishFilterChange()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    endFilterChange(Direction::Rows);
#else
    invalidateRowsFilter();
#endif
}
