// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file subtypesmodel.h
/// \brief Declares the table model of a type's subtypes.
///

#pragma once

#include <QHash>

#include "data/typeinfo.h"
#include "itemtablemodel.h"

///
/// \brief Table of type definitions: name, node id, category and child count.
///
class SubtypesModel : public ItemTableModel<TypeInfo>
{
    Q_OBJECT

public:
    ///
    /// \brief Columns exposed by the model.
    ///
    enum Column { ColName, ColNodeId, ColCategory, ColChildren, ColCount };

    ///
    /// \brief Constructs an empty model.
    /// \param parent Owning QObject.
    ///
    explicit SubtypesModel(QObject *parent = nullptr);

    ///
    /// \brief Replaces the subtypes and the icons of their names.
    /// \param subtypes Subtypes in row order.
    /// \param iconNames Icon resource names keyed by type node id; a type without one shows no icon.
    ///
    void setSubtypes(const QList<TypeInfo> &subtypes, const QHash<QString, QString> &iconNames);

    ///
    /// \brief Returns the fixed column count.
    /// \param parent Parent index; non-root parents have no columns.
    /// \return Column count, or 0 for non-root parents.
    ///
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns cell text and the type icon; abstract types are muted.
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

private:
    QHash<QString, QString> _iconNames;
};
