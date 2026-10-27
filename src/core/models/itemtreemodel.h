// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file itemtreemodel.h
/// \brief Declares the base for hierarchical models backed by a flat list of plain structs.
///

#pragma once

#include <QAbstractItemModel>
#include <QHash>
#include <QList>

///
/// \brief Tree model storing one struct per row, keyed by its \c nodeId; subclasses map fields to columns.
///
/// Items keep their source order; an index carries the source row plus one, so 0 stays free for the root.
///
template <typename T>
class ItemTreeModel : public QAbstractItemModel
{
public:
    using QAbstractItemModel::QAbstractItemModel;

    ///
    /// \brief Returns the number of children of an item or the root.
    /// \param parent Parent index; only first-column cells have children.
    /// \return Child count.
    ///
    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.isValid() && parent.column() != 0)
            return 0;
        return int(_children.value(parent.isValid() ? sourceRow(parent) : -1).size());
    }

    ///
    /// \brief Returns a cell among the children of an item or the root.
    /// \param row Child row.
    /// \param column Column.
    /// \param parent Parent index.
    /// \return Cell index, or an invalid index out of range.
    ///
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override
    {
        if (!hasIndex(row, column, parent))
            return {};
        const int source = _children.value(parent.isValid() ? sourceRow(parent) : -1).at(row);
        return createIndex(row, column, quintptr(source + 1));
    }

    ///
    /// \brief Returns the item owning a child row.
    /// \param index Child cell.
    /// \return Owner cell in the first column, or an invalid index for a top-level row.
    ///
    QModelIndex parent(const QModelIndex &index) const override
    {
        if (!index.isValid())
            return {};
        return indexForRow(_parents.at(sourceRow(index)));
    }

    ///
    /// \brief Maps a stored item to its hierarchical cell.
    /// \param row Source row.
    /// \param column Column.
    /// \return Cell index, or an invalid index for a row out of range.
    ///
    QModelIndex indexForRow(int row, int column = 0) const
    {
        if (row < 0 || row >= _items.size())
            return {};
        return createIndex(int(_children.value(_parents.at(row)).indexOf(row)), column, quintptr(row + 1));
    }

    ///
    /// \brief Finds the source row of an item.
    /// \param nodeId Item node id.
    /// \return Source row, or -1 when the item is not stored.
    ///
    int rowOf(const QString &nodeId) const
    {
        for (int row = 0; row < _items.size(); ++row) {
            if (_items.at(row).nodeId == nodeId)
                return row;
        }
        return -1;
    }

    ///
    /// \brief Replaces all items, resetting attached views.
    /// \param items Items in source order; children keep this order below their owner.
    /// \param owners Owner node ids keyed by child node id; absent owners leave items at the root.
    ///
    void setItems(const QList<T> &items, const QHash<QString, QString> &owners = {})
    {
        beginResetModel();
        _items = items;
        _parents.clear();
        _children.clear();
        QHash<QString, int> rows;
        for (int row = int(_items.size()) - 1; row >= 0; --row)
            rows.insert(_items.at(row).nodeId, row);
        for (int row = 0; row < _items.size(); ++row) {
            const int owner = rows.value(owners.value(_items.at(row).nodeId), -1);
            _parents.append(owner);
            _children[owner].append(row);
        }
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
    /// \return Items in source order.
    ///
    const QList<T> &items() const
    {
        return _items;
    }

    ///
    /// \brief Returns a stored item.
    /// \param row Valid source row.
    /// \return Item of the row.
    ///
    const T &item(int row) const
    {
        return _items.at(row);
    }

    ///
    /// \brief Replaces a stored item without changing its owner and announces the change of its cells.
    /// \param row Valid source row.
    /// \param item New item.
    ///
    void setItem(int row, const T &item)
    {
        _items[row] = item;
        emit this->dataChanged(indexForRow(row), indexForRow(row, this->columnCount() - 1));
    }

    ///
    /// \brief Makes attached views fetch the headers and cells again, e.g. after the language changed.
    ///
    void retranslate()
    {
        const int columns = this->columnCount();
        if (columns == 0)
            return;
        emit this->headerDataChanged(Qt::Horizontal, 0, columns - 1);
        for (int row = 0; row < _items.size(); ++row)
            emit this->dataChanged(indexForRow(row), indexForRow(row, columns - 1));
    }

protected:
    ///
    /// \brief Reports whether an index addresses a stored item of this model.
    /// \param index Index to check.
    /// \return True for a valid index of this model within the stored rows.
    ///
    bool hasItem(const QModelIndex &index) const
    {
        return index.isValid() && index.model() == this && index.internalId() > 0
            && index.internalId() <= quintptr(_items.size());
    }

    ///
    /// \brief Returns the source row of a cell.
    /// \param index Cell for which hasItem() holds.
    /// \return Source row.
    ///
    static int sourceRow(const QModelIndex &index)
    {
        return int(index.internalId()) - 1;
    }

private:
    QList<T> _items;
    QList<int> _parents;
    QHash<int, QList<int>> _children;
};
