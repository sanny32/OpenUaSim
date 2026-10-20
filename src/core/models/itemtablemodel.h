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
    /// \brief Makes attached views fetch the headers and cells again, e.g. after the language changed;
    ///        the selection is kept.
    ///
    void retranslate()
    {
        const int columns = this->columnCount();
        if (columns == 0)
            return;
        emit this->headerDataChanged(Qt::Horizontal, 0, columns - 1);
        if (!_items.isEmpty())
            emit this->dataChanged(this->index(0, 0), this->index(int(_items.size()) - 1, columns - 1));
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

    ///
    /// \brief Replaces the item of a row and announces the change of every cell of the row.
    /// \param row Valid row number.
    /// \param item New item.
    ///
    void setItem(int row, const T &item)
    {
        _items[row] = item;
        emit this->dataChanged(this->index(row, 0), this->index(row, this->columnCount() - 1));
    }

    ///
    /// \brief Appends rows and, beyond a limit, drops the oldest ones.
    /// \param items Items to append.
    /// \param limit Maximum row count; 0 keeps every row.
    ///
    void appendItems(const QList<T> &items, int limit = 0)
    {
        if (items.isEmpty())
            return;
        const int first = static_cast<int>(_items.size());
        this->beginInsertRows(QModelIndex(), first, first + static_cast<int>(items.size()) - 1);
        _items.append(items);
        this->endInsertRows();
        const int excess = limit > 0 ? static_cast<int>(_items.size()) - limit : 0;
        if (excess > 0) {
            this->beginRemoveRows(QModelIndex(), 0, excess - 1);
            _items.remove(0, excess);
            this->endRemoveRows();
        }
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
