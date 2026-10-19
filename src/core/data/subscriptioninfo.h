// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file subscriptioninfo.h
/// \brief Declares the description of a client subscription.
///

#pragma once

#include <QtGlobal>

///
/// \brief Describes one subscription a session holds.
///
struct SubscriptionInfo
{
    /// \brief Server-assigned subscription id.
    quint32 id = 0;
    /// \brief Revised publishing interval in milliseconds.
    double publishingIntervalMs = 0.0;
    /// \brief Whether publishing is enabled.
    bool publishingEnabled = false;
    /// \brief Relative priority the client requested.
    int priority = 0;
    /// \brief Monitored items of the subscription.
    quint32 monitoredItems = 0;
    /// \brief Notifications sent so far.
    quint32 notifications = 0;
    /// \brief Keep-alive count the server revised.
    quint32 maxKeepAliveCount = 0;
    /// \brief Lifetime count the server revised.
    quint32 maxLifetimeCount = 0;

    ///
    /// \brief Compares every field.
    /// \param other Subscription to compare with.
    /// \return True when all fields are equal.
    ///
    bool operator==(const SubscriptionInfo &other) const
    {
        return id == other.id && publishingIntervalMs == other.publishingIntervalMs
            && publishingEnabled == other.publishingEnabled && priority == other.priority
            && monitoredItems == other.monitoredItems && notifications == other.notifications
            && maxKeepAliveCount == other.maxKeepAliveCount && maxLifetimeCount == other.maxLifetimeCount;
    }
};
