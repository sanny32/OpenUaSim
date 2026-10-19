// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file serverstatus.h
/// \brief Declares the summary of the server state.
///

#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>

///
/// \brief Summary of the server state shown on the status page and status bar.
///
struct ServerStatus
{
    /// \brief Whether the server is running.
    bool running = false;
    /// \brief URL of the primary UA TCP endpoint.
    QString primaryEndpoint;
    /// \brief Server application URI.
    QString applicationUri;
    /// \brief Time the server was started.
    QDateTime startTime;
    /// \brief Seconds elapsed since the start.
    qint64 uptimeSeconds = 0;
    /// \brief Enabled message security modes.
    QStringList securityModes;
    /// \brief Number of nodes in the address space.
    int totalNodes = 0;
    /// \brief Number of registered namespaces.
    int namespaces = 0;
    /// \brief Number of connected clients.
    int clientsConnected = 0;
};
