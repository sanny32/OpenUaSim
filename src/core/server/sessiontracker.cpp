// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file sessiontracker.cpp
/// \brief Implements the record of active and recently closed client sessions.
///

#include "sessiontracker.h"

///
/// \brief Records a new active session.
/// \param session Session description; its active flag is forced on.
///
void SessionTracker::open(const SessionInfo &session)
{
    SessionInfo active = session;
    active.active = true;
    _active.prepend(active);
}

///
/// \brief Marks a session closed.
/// \param sessionId Identifier of the session.
/// \param time Time the session closed.
/// \return False when no active session has that identifier.
///
bool SessionTracker::close(const QString &sessionId, const QDateTime &time)
{
    for (qsizetype index = 0; index < _active.size(); ++index) {
        if (_active.at(index).sessionId != sessionId)
            continue;
        SessionInfo closed = _active.takeAt(index);
        closed.active = false;
        closed.lastActivity = time;
        _closed.prepend(closed);
        trimHistory();
        return true;
    }
    return false;
}

///
/// \brief Updates the live counters of an active session.
/// \param sessionId Identifier of the session.
/// \param subscriptions Number of subscriptions it holds.
/// \param lastActivity Time of its last request.
/// \return True when the session is active and a value changed.
///
bool SessionTracker::update(const QString &sessionId, int subscriptions, const QDateTime &lastActivity)
{
    for (SessionInfo &session : _active) {
        if (session.sessionId != sessionId)
            continue;
        if (session.subscriptions == subscriptions && session.lastActivity == lastActivity)
            return false;
        session.subscriptions = subscriptions;
        session.lastActivity = lastActivity;
        return true;
    }
    return false;
}

///
/// \brief Marks every active session closed, e.g. when the server stops.
/// \param time Time the sessions closed.
///
void SessionTracker::closeAll(const QDateTime &time)
{
    while (!_active.isEmpty())
        close(_active.last().sessionId, time);
}

///
/// \brief Forgets every session.
///
void SessionTracker::clear()
{
    _active.clear();
    _closed.clear();
}

///
/// \brief Returns the sessions, active ones first, each group newest first.
/// \return Session list.
///
QList<SessionInfo> SessionTracker::sessions() const
{
    return _active + _closed;
}

///
/// \brief Returns the number of active sessions.
/// \return Active session count.
///
int SessionTracker::activeCount() const
{
    return int(_active.size());
}

///
/// \brief Drops the oldest closed sessions beyond the history limit.
///
void SessionTracker::trimHistory()
{
    while (_closed.size() > kClosedHistory)
        _closed.removeLast();
}
