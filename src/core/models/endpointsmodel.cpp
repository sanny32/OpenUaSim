// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file endpointsmodel.cpp
/// \brief Implements the table model of server endpoints.
///

#include "endpointsmodel.h"
#include "itemroles.h"

///
/// \brief Constructs an empty model.
/// \param parent Owning QObject.
///
EndpointsModel::EndpointsModel(QObject *parent)
    : ItemTableModel(parent)
{
}

///
/// \brief Returns the fixed column count.
/// \param parent Parent index; non-root parents have no columns.
/// \return Column count, or 0 for non-root parents.
///
int EndpointsModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColCount;
}

///
/// \brief Returns cell text and the enabled-state indicator.
/// \param index Cell to query.
/// \param role Requested data role.
/// \return Value for the role, or an invalid variant.
///
QVariant EndpointsModel::data(const QModelIndex &index, int role) const
{
    if (!hasItem(index))
        return {};
    const EndpointInfo &endpoint = item(index.row());

    if (role == ItemRoles::Id)
        return endpoint.url();
    if (role == ItemRoles::Status && index.column() == ColEnabled)
        return ItemRoles::statusValue(endpoint.enabled ? StatusKind::Good : StatusKind::Inactive);
    if (role != Qt::DisplayRole)
        return {};

    switch (index.column()) {
    case ColProtocol:       return endpoint.protocol;
    case ColUrl:            return endpoint.url();
    case ColSecurityPolicy: return endpoint.securityPolicy;
    case ColSecurityMode:   return endpoint.securityMode;
    case ColEnabled:        return endpoint.enabled ? tr("Yes") : tr("No");
    default:                return {};
    }
}

///
/// \brief Returns the column titles.
/// \param section Column index.
/// \param orientation Header orientation.
/// \param role Requested data role.
/// \return Column title, or the base implementation otherwise.
///
QVariant EndpointsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return ItemTableModel::headerData(section, orientation, role);
    switch (section) {
    case ColProtocol:       return tr("Protocol");
    case ColUrl:            return tr("URL");
    case ColSecurityPolicy: return tr("Security Policy");
    case ColSecurityMode:   return tr("Security Mode");
    case ColEnabled:        return tr("Enabled");
    default:                return {};
    }
}
