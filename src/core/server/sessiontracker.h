// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file sessiontracker.h
/// \brief Declares the record of active and recently closed client sessions.
///

#pragma once

#include <QList>

#include "data/sessioninfo.h"

///
/// \brief Keeps active sessions and a bounded history of closed ones.
///
class SessionTracker
{
public:
    /// \brief Number of closed sessions kept; older ones are dropped first.
    static constexpr int kClosedHistory = 100;

    ///
    /// \brief Records a new active session.
    /// \param session Session description; its active flag is forced on.
    ///
    void open(const SessionInfo &session);

    ///
    /// \brief Marks a session closed.
    /// \param sessionId Identifier of the session.
    /// \param time Time the session closed.
    /// \return False when no active session has that identifier.
    ///
    bool close(const QString &sessionId, const QDateTime &time);

    ///
    /// \brief Updates the live counters of an active session.
    /// \param sessionId Identifier of the session.
    /// \param subscriptions Subscriptions it holds.
    /// \param lastActivity Time of its last request.
    /// \return True when the session is active and a value changed.
    ///
    bool update(const QString &sessionId, const QList<SubscriptionInfo> &subscriptions, const QDateTime &lastActivity);

    ///
    /// \brief Marks every active session closed, e.g. when the server stops.
    /// \param time Time the sessions closed.
    ///
    void closeAll(const QDateTime &time);

    ///
    /// \brief Forgets every session.
    ///
    void clear();

    ///
    /// \brief Returns the sessions, active ones first, each group newest first.
    /// \return Session list.
    ///
    QList<SessionInfo> sessions() const;

    ///
    /// \brief Returns the number of active sessions.
    /// \return Active session count.
    ///
    int activeCount() const;

private:
    void trimHistory();

    QList<SessionInfo> _active;
    QList<SessionInfo> _closed;
};
