// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file subtypesmodel.h
/// \brief Declares the hierarchical model of a type's subtypes.
///

#pragma once

#include <QHash>

#include "data/typeinfo.h"
#include "itemtreemodel.h"

///
/// \brief Hierarchy of type definitions below their supertype: name, node id, category and child count.
///
class SubtypesModel : public ItemTreeModel<TypeInfo>
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
    /// \param subtypes Subtypes in row order; nested ones follow their supertype.
    /// \param supertypes Supertype node ids keyed by the node id of a nested subtype; others are top-level rows.
    /// \param iconNames Icon resource names keyed by type node id; a type without one shows no icon.
    ///
    void setSubtypes(const QList<TypeInfo> &subtypes, const QHash<QString, QString> &supertypes,
                     const QHash<QString, QString> &iconNames);

    ///
    /// \brief Returns the fixed column count.
    /// \param parent Unused; every row has the same columns.
    /// \return Column count.
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
