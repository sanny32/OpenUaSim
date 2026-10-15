// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file endpointinfo.h
/// \brief Declares the description of a server endpoint.
///

#pragma once

#include <QString>

///
/// \brief Describes one endpoint the server listens on.
///
struct EndpointInfo
{
    /// \brief User-facing endpoint name.
    QString name;
    /// \brief Resource name of the icon.
    QString icon;
    /// \brief URL scheme, e.g. "opc.tcp".
    QString protocol;
    /// \brief Host name or address.
    QString hostname;
    /// \brief TCP port.
    int port = 4840;
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
    /// \brief Whether clients must present a certificate.
    bool requireClientCertificate = false;

    ///
    /// \brief Builds the endpoint URL from its parts.
    /// \return URL such as "opc.tcp://localhost:4840/discovery".
    ///
    QString url() const
    {
        return QStringLiteral("%1://%2:%3%4").arg(protocol, hostname).arg(port).arg(path);
    }
};
