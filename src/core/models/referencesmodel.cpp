// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file referencesmodel.cpp
/// \brief Implements the table model of reference counts grouped by type.
///

#include "referencesmodel.h"

///
/// \brief Constructs an empty model.
/// \param parent Owning QObject.
///
ReferencesModel::ReferencesModel(QObject *parent)
    : ItemTableModel(parent)
{
}

///
/// \brief Returns the fixed column count.
/// \param parent Parent index; non-root parents have no columns.
/// \return Column count, or 0 for non-root parents.
///
int ReferencesModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColCount;
}

///
/// \brief Returns the reference type name and count.
/// \param index Cell to query.
/// \param role Requested data role.
/// \return Value for the role, or an invalid variant.
///
QVariant ReferencesModel::data(const QModelIndex &index, int role) const
{
    if (!hasItem(index) || role != Qt::DisplayRole)
        return {};
    const ReferenceCount &reference = item(index.row());
    return index.column() == ColReferenceType ? QVariant(reference.referenceType)
                                              : QVariant(reference.count);
}

///
/// \brief Returns the column titles.
/// \param section Column index.
/// \param orientation Header orientation.
/// \param role Requested data role.
/// \return Column title, or the base implementation otherwise.
///
QVariant ReferencesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return ItemTableModel::headerData(section, orientation, role);
    switch (section) {
    case ColReferenceType: return tr("Reference");
    case ColReferences:    return tr("Count");
    default:               return {};
    }
}
