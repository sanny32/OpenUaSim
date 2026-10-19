// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file subscriptionsmodel.cpp
/// \brief Implements the table model of the subscriptions of a session.
///

#include "formatting.h"
#include "subscriptionsmodel.h"

///
/// \brief Constructs an empty model.
/// \param parent Owning QObject.
///
SubscriptionsModel::SubscriptionsModel(QObject *parent)
    : ItemTableModel(parent)
{
}

///
/// \brief Returns the fixed column count.
/// \param parent Parent index; non-root parents have no columns.
/// \return Column count, or 0 for non-root parents.
///
int SubscriptionsModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColCount;
}

///
/// \brief Returns cell text.
/// \param index Cell to query.
/// \param role Requested data role.
/// \return Display text, or an invalid variant for other roles.
///
QVariant SubscriptionsModel::data(const QModelIndex &index, int role) const
{
    if (!hasItem(index) || role != Qt::DisplayRole)
        return {};
    const SubscriptionInfo &subscription = item(index.row());
    switch (index.column()) {
    case ColId:                 return subscription.id;
    case ColPublishingInterval: return tr("%1 ms").arg(Formatting::decimal(subscription.publishingIntervalMs));
    case ColPublishing:         return subscription.publishingEnabled ? tr("Enabled") : tr("Disabled");
    case ColPriority:           return subscription.priority;
    case ColMonitoredItems:     return subscription.monitoredItems;
    case ColNotifications:      return subscription.notifications;
    case ColKeepAlive:          return subscription.maxKeepAliveCount;
    case ColLifetime:           return subscription.maxLifetimeCount;
    default:                    return {};
    }
}

///
/// \brief Returns the column titles.
/// \param section Column index.
/// \param orientation Header orientation.
/// \param role Requested data role.
/// \return Column title, or the base implementation otherwise.
///
QVariant SubscriptionsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return ItemTableModel::headerData(section, orientation, role);
    switch (section) {
    case ColId:                 return tr("ID");
    case ColPublishingInterval: return tr("Publishing Interval");
    case ColPublishing:         return tr("Publishing");
    case ColPriority:           return tr("Priority");
    case ColMonitoredItems:     return tr("Monitored Items");
    case ColNotifications:      return tr("Notifications");
    case ColKeepAlive:          return tr("Max Keep-Alive");
    case ColLifetime:           return tr("Max Lifetime");
    default:                    return {};
    }
}
