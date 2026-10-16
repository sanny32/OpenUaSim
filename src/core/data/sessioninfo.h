// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file sessioninfo.h
/// \brief Declares the description of a client session.
///

#pragma once

#include <QDateTime>
#include <QString>

///
/// \brief Describes one client session, active or closed.
///
struct SessionInfo
{
    /// \brief String form of the session node identifier.
    QString sessionId;
    /// \brief Session name sent by the client.
    QString name;
    /// \brief Client product and version.
    QString client;
    /// \brief Authenticated user name.
    QString user;
    /// \brief Whether the session is still open.
    bool active = false;
    /// \brief Number of subscriptions the session holds.
    int subscriptions = 0;
    /// \brief Session creation time.
    QDateTime created;
    /// \brief Time of the last request.
    QDateTime lastActivity;
    /// \brief Client application URI.
    QString applicationUri;
    /// \brief Endpoint URL the client connected to.
    QString endpointUrl;
    /// \brief Message security mode text.
    QString securityMode;
    /// \brief User identity token type text.
    QString userIdentity;
};
