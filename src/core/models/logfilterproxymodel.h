// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file logfilterproxymodel.h
/// \brief Declares the level, category and text filter over LogModel.
///

#pragma once

#include <QSet>
#include <QSortFilterProxyModel>

#include "data/logentry.h"

class LogModel;

///
/// \brief Hides LogModel rows whose level or category is switched off or whose text
///        does not match the search string.
///
/// Every level and category is visible until switched off. The search string, set with
/// setFilterFixedString(), is matched case-insensitively against every column.
///
class LogFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs a filter showing every entry.
    /// \param parent Owning QObject.
    ///
    explicit LogFilterProxyModel(QObject *parent = nullptr);

    ///
    /// \brief Shows or hides entries of a level.
    /// \param level Level to toggle.
    /// \param visible True to show the level.
    ///
    void setLevelVisible(LogEntry::Level level, bool visible);

    ///
    /// \brief Shows or hides entries of a category.
    /// \param category Category to toggle.
    /// \param visible True to show the category.
    ///
    void setCategoryVisible(LogEntry::Category category, bool visible);

    ///
    /// \brief Reports whether entries of a level are shown.
    /// \param level Level to check.
    /// \return True when the level is visible.
    ///
    bool isLevelVisible(LogEntry::Level level) const;

    ///
    /// \brief Reports whether entries of a category are shown.
    /// \param category Category to check.
    /// \return True when the category is visible.
    ///
    bool isCategoryVisible(LogEntry::Category category) const;

    ///
    /// \brief Returns the log entry behind a proxy row.
    /// \param index Proxy index; any column of the row.
    /// \return Pointer to the entry, or nullptr when the index is invalid
    ///         or the source is not a LogModel.
    ///
    const LogEntry *entry(const QModelIndex &index) const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    void finishFilterChange();
    const LogModel *logModel() const;

    QSet<LogEntry::Level> _hiddenLevels;
    QSet<LogEntry::Category> _hiddenCategories;
};
