// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file namespacesmodel.cpp
/// \brief Implements the table model of the namespace array.
///

#include "itemroles.h"
#include "namespacesmodel.h"

///
/// \brief Constructs an empty model.
/// \param parent Owning QObject.
///
NamespacesModel::NamespacesModel(QObject *parent)
    : ItemTableModel(parent)
{
}

///
/// \brief Returns the fixed column count.
/// \param parent Parent index; non-root parents have no columns.
/// \return Column count, or 0 for non-root parents.
///
int NamespacesModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColCount;
}

///
/// \brief Returns cell text.
/// \param index Cell to query.
/// \param role Requested data role.
/// \return Value for the role, or an invalid variant.
///
QVariant NamespacesModel::data(const QModelIndex &index, int role) const
{
    if (!hasItem(index))
        return {};
    const NamespaceInfo &ns = item(index.row());

    if (role == ItemRoles::Id)
        return ns.uri;
    if (role != Qt::DisplayRole)
        return {};

    switch (index.column()) {
    case ColIndex:           return ns.index;
    case ColUri:             return ns.uri;
    case ColVersion:         return ns.version;
    case ColPublicationDate: return ns.publicationDate.toString(Qt::ISODate);
    default:                 return {};
    }
}

///
/// \brief Returns the column titles.
/// \param section Column index.
/// \param orientation Header orientation.
/// \param role Requested data role.
/// \return Column title, or the base implementation otherwise.
///
QVariant NamespacesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return ItemTableModel::headerData(section, orientation, role);
    switch (section) {
    case ColIndex:           return tr("Index");
    case ColUri:             return tr("URI");
    case ColVersion:         return tr("Version");
    case ColPublicationDate: return tr("Publication Date");
    default:                 return {};
    }
}
