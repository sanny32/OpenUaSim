// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file variablesmodel.h
/// \brief Declares the hierarchical model of an object's variables.
///

#pragma once

#include <QAbstractItemModel>
#include <QHash>

#include "data/statuskind.h"
#include "data/variableinfo.h"

///
/// \brief Hierarchy of the variables of one object: name, value, data type, node type, quality and alarm state.
///
class VariablesModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    ///
    /// \brief Columns exposed by the model.
    ///
    enum Column { ColName, ColValue, ColDataType, ColNodeType, ColQuality, ColAlarm, ColCount };

    ///
    /// \brief Constructs an empty model.
    /// \param parent Owning QObject.
    ///
    explicit VariablesModel(QObject *parent = nullptr);

    ///
    /// \brief Returns the fixed column count.
    /// \param parent Parent index.
    /// \return Column count.
    ///
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns cell text, the variable icon, and the quality and alarm indicators.
    /// \param index Cell to query.
    /// \param role Requested data role.
    /// \return Value for the role, or an invalid variant.
    ///
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    ///
    /// \brief Returns the column titles.
    /// \param section Column index.
    /// \param orientation Header orientation.
    /// \param role Requested data role.
    /// \return Column title, or the base implementation otherwise.
    ///
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    ///
    /// \brief Finds the row of a variable.
    /// \param nodeId Variable node id.
    /// \return Row number, or -1 when the variable is not listed.
    ///
    int rowOf(const QString &nodeId) const;

    ///
    /// \brief Replaces the listed variables.
    /// \param variables Variables in row order.
    /// \param locations Owner path shown before the name, keyed by variable node id; omitted for none.
    /// \param owners Parent variable ids keyed by child node id; absent parents leave rows at the root.
    ///
    void setVariables(const QList<VariableInfo> &variables, const QHash<QString, QString> &locations = {},
                      const QHash<QString, QString> &owners = {});

    ///
    /// \brief Updates the values of the listed variables.
    /// \param values New values keyed by node id; variables not listed are ignored.
    ///
    void updateValues(const QHash<QString, QVariant> &values);

    ///
    /// \brief Returns the display text of a quality.
    /// \param quality Quality to describe.
    /// \return Severity, followed by the reason of a specific status code, e.g. "Bad (sensor failure)".
    ///
    static QString qualityText(VariableInfo::Quality quality);

    ///
    /// \brief Returns the indicator state of a quality.
    /// \param quality Quality to describe.
    /// \return Good, Warning or Error.
    ///
    static StatusKind qualityStatus(VariableInfo::Quality quality);

    ///
    /// \brief Returns the number of children of a variable or the root.
    ///
    int rowCount(const QModelIndex &parent = {}) const override;

    ///
    /// \brief Returns a cell among the children of a variable or the root.
    ///
    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;

    ///
    /// \brief Returns the variable owning a child row.
    ///
    QModelIndex parent(const QModelIndex &index) const override;

    ///
    /// \brief Maps a stored item to its hierarchical cell.
    ///
    QModelIndex indexForRow(int row, int column = 0) const;

    ///
    /// \brief Replaces the items with a flat list.
    ///
    void setItems(const QList<VariableInfo> &items);

    ///
    /// \brief Returns the stored items in source order.
    ///
    const QList<VariableInfo> &items() const;

    ///
    /// \brief Returns a stored item by its source row.
    ///
    const VariableInfo &item(int row) const;

    ///
    /// \brief Updates a stored item without changing its parent.
    ///
    void setItem(int row, const VariableInfo &item);

    ///
    /// \brief Removes all stored items.
    ///
    void clear();

    ///
    /// \brief Refreshes translated headers and cells.
    ///
    void retranslate();

private:
    QList<VariableInfo> _items;
    QList<int> _parents;
    QHash<int, QList<int>> _children;
    QHash<QString, QString> _locations;
};
