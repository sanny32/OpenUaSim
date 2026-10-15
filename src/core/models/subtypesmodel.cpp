// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file subtypesmodel.cpp
/// \brief Implements the table model of a type's subtypes.
///

#include "itemroles.h"
#include "subtypesmodel.h"

///
/// \brief Constructs an empty model.
/// \param parent Owning QObject.
///
SubtypesModel::SubtypesModel(QObject *parent)
    : ItemTableModel(parent)
{
}

///
/// \brief Returns the fixed column count.
/// \param parent Parent index; non-root parents have no columns.
/// \return Column count, or 0 for non-root parents.
///
int SubtypesModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColCount;
}

///
/// \brief Returns cell text and the type icon.
/// \param index Cell to query.
/// \param role Requested data role.
/// \return Value for the role, or an invalid variant.
///
QVariant SubtypesModel::data(const QModelIndex &index, int role) const
{
    if (!hasItem(index))
        return {};
    const TypeInfo &type = item(index.row());

    if (role == ItemRoles::Id)
        return type.nodeId;
    if (role == ItemRoles::IconName && index.column() == ColName)
        return QStringLiteral("folder");
    if (role != Qt::DisplayRole)
        return {};

    switch (index.column()) {
    case ColName:     return type.browseName;
    case ColNodeId:   return type.nodeId;
    case ColCategory: return type.category;
    case ColChildren: return type.children;
    default:          return {};
    }
}

///
/// \brief Returns the column titles.
/// \param section Column index.
/// \param orientation Header orientation.
/// \param role Requested data role.
/// \return Column title, or the base implementation otherwise.
///
QVariant SubtypesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return ItemTableModel::headerData(section, orientation, role);
    switch (section) {
    case ColName:     return tr("Name");
    case ColNodeId:   return tr("NodeId");
    case ColCategory: return tr("Category");
    case ColChildren: return tr("Children");
    default:          return {};
    }
}
