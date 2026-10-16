// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file itemtablemodel.h
/// \brief Declares the base for table models backed by a list of plain structs.
///

#pragma once

#include <QAbstractTableModel>
#include <QList>

///
/// \brief Table model storing one struct per row; subclasses map fields to columns.
///
template <typename T>
class ItemTableModel : public QAbstractTableModel
{
public:
    using QAbstractTableModel::QAbstractTableModel;

    ///
    /// \brief Returns the number of stored items.
    /// \param parent Parent index; non-root parents have no rows.
    /// \return Item count, or 0 for non-root parents.
    ///
    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : static_cast<int>(_items.size());
    }

    ///
    /// \brief Replaces all items, resetting attached views.
    /// \param items New items.
    ///
    void setItems(const QList<T> &items)
    {
        beginResetModel();
        _items = items;
        endResetModel();
    }

    ///
    /// \brief Removes all items.
    ///
    void clear()
    {
        setItems({});
    }

    ///
    /// \brief Returns all stored items.
    /// \return Items in row order.
    ///
    const QList<T> &items() const
    {
        return _items;
    }

    ///
    /// \brief Returns the item of a row.
    /// \param row Valid row number.
    /// \return Item of the row.
    ///
    const T &item(int row) const
    {
        return _items.at(row);
    }

protected:
    ///
    /// \brief Reports whether an index addresses a stored item.
    /// \param index Index to check.
    /// \return True for a valid index within the stored rows.
    ///
    bool hasItem(const QModelIndex &index) const
    {
        return index.isValid() && index.row() >= 0 && index.row() < _items.size();
    }

private:
    QList<T> _items;
};
