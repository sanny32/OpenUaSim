// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file endpointinfo.h
/// \brief Declares the description of a server endpoint.
///

#pragma once

#include <QString>

/// \brief Port of new endpoints; not the IANA 4840 so the simulator can run next to a real server.
constexpr int kDefaultEndpointPort = 48400;

///
/// \brief Describes one endpoint the server listens on.
///
struct EndpointInfo
{
    /// \brief URL scheme, e.g. "opc.tcp".
    QString protocol;
    /// \brief Host name or address.
    QString hostname;
    /// \brief TCP port.
    int port = kDefaultEndpointPort;
    /// \brief Optional URL path, starting with a slash.
    QString path;
    /// \brief Security policy name.
    QString securityPolicy;
    /// \brief Message security mode text.
    QString securityMode;
    /// \brief Whether the endpoint is enabled.
    bool enabled = true;
    /// \brief Whether anonymous user tokens are accepted.
    bool allowAnonymous = true;
    /// \brief Whether username/password user tokens are accepted.
    bool usernamePassword = false;
    /// \brief User name for username/password tokens.
    QString username;
    /// \brief Password for username/password tokens.
    QString password;
    /// \brief Whether only clients with a trusted certificate are accepted; untrusted ones are rejected.
    bool trustedClientsOnly = false;

    ///
    /// \brief Builds the endpoint URL from its parts.
    /// \return URL such as "opc.tcp://localhost:48400/discovery".
    ///
    QString url() const
    {
        return QStringLiteral("%1://%2:%3%4").arg(protocol, hostname).arg(port).arg(path);
    }
};
