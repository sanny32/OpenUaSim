// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file subscriptionsmodel.h
/// \brief Declares the table model of the subscriptions of a session.
///

#pragma once

#include "data/subscriptioninfo.h"
#include "itemtablemodel.h"

///
/// \brief Table of subscriptions: id, publishing, monitored items, notifications and keep-alive settings.
///
class SubscriptionsModel : public ItemTableModel<SubscriptionInfo>
{
    Q_OBJECT

public:
    ///
    /// \brief Columns exposed by the model.
    ///
    enum Column {
        ColId,
        ColPublishingInterval,
        ColPublishing,
        ColPriority,
        ColMonitoredItems,
        ColNotifications,
        ColKeepAlive,
        ColLifetime,
        ColCount
    };

    ///
    /// \brief Constructs an empty model.
    /// \param parent Owning QObject.
    ///
    explicit SubscriptionsModel(QObject *parent = nullptr);

    ///
    /// \brief Returns the fixed column count.
    /// \param parent Parent index; non-root parents have no columns.
    /// \return Column count, or 0 for non-root parents.
    ///
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns cell text.
    /// \param index Cell to query.
    /// \param role Requested data role.
    /// \return Display text, or an invalid variant for other roles.
    ///
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    ///
    /// \brief Returns the column titles.
    /// \param section Column index.
    /// \param orientation Header orientation.
    /// \param role Requested data role.
    /// \return Column title, or the base implementation otherwise.
    ///
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
};
