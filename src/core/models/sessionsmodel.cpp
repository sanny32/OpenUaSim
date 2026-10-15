// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file sessionsmodel.cpp
/// \brief Implements the table model of client sessions.
///

#include "formatting.h"
#include "itemroles.h"
#include "sessionsmodel.h"

///
/// \brief Constructs an empty model.
/// \param parent Owning QObject.
///
SessionsModel::SessionsModel(QObject *parent)
    : ItemTableModel(parent)
{
}

///
/// \brief Returns the fixed column count.
/// \param parent Parent index; non-root parents have no columns.
/// \return Column count, or 0 for non-root parents.
///
int SessionsModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColCount;
}

///
/// \brief Returns cell text, the session icon, and the state indicator.
/// \param index Cell to query.
/// \param role Requested data role.
/// \return Value for the role, or an invalid variant.
///
QVariant SessionsModel::data(const QModelIndex &index, int role) const
{
    if (!hasItem(index))
        return {};
    const SessionInfo &session = item(index.row());

    if (role == ItemRoles::Id)
        return session.sessionId;
    if (role == ItemRoles::IconName && index.column() == ColName)
        return QStringLiteral("user");
    if (role == ItemRoles::Status && index.column() == ColState)
        return ItemRoles::statusValue(session.active ? StatusKind::Good : StatusKind::Inactive);
    if (role != Qt::DisplayRole)
        return {};

    switch (index.column()) {
    case ColName:          return session.name;
    case ColClient:        return session.client;
    case ColUser:          return session.user;
    case ColState:         return stateText(session.active);
    case ColSubscriptions: return session.subscriptions;
    case ColLastActivity:  return Formatting::dateTime(session.lastActivity);
    default:               return {};
    }
}

///
/// \brief Returns the column titles.
/// \param section Column index.
/// \param orientation Header orientation.
/// \param role Requested data role.
/// \return Column title, or the base implementation otherwise.
///
QVariant SessionsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return ItemTableModel::headerData(section, orientation, role);
    switch (section) {
    case ColName:          return tr("Session Name");
    case ColClient:        return tr("Client");
    case ColUser:          return tr("User");
    case ColState:         return tr("State");
    case ColSubscriptions: return tr("Subscriptions");
    case ColLastActivity:  return tr("Last Activity");
    default:               return {};
    }
}

///
/// \brief Finds the row of a session.
/// \param sessionId Session node id.
/// \return Row number, or -1 when the session is not listed.
///
int SessionsModel::rowOf(const QString &sessionId) const
{
    for (int row = 0; row < items().size(); ++row) {
        if (items().at(row).sessionId == sessionId)
            return row;
    }
    return -1;
}

///
/// \brief Returns the state text of a session.
/// \param active Whether the session is open.
/// \return "Active" or "Closed".
///
QString SessionsModel::stateText(bool active)
{
    return active ? tr("Active") : tr("Closed");
}
