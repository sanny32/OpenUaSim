// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file listenersettings.h
/// \brief Declares how the configured endpoints collapse onto the single listener QUaServer provides.
///

#pragma once

#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

#include "data/endpointinfo.h"

///
/// \brief Settings of the one TCP listener all enabled endpoints share.
///
struct ListenerSettings
{
    /// \brief Host name advertised in endpoint URLs, taken from the first enabled endpoint.
    QString hostname;
    /// \brief TCP port shared by every enabled endpoint.
    quint16 port = kDefaultEndpointPort;
    /// \brief Whether any enabled endpoint accepts anonymous user tokens.
    bool anonymousAllowed = false;
    /// \brief User names and passwords of the endpoints accepting username tokens.
    QList<QPair<QString, QString>> users;
    /// \brief Distinct security policy names of the enabled endpoints, e.g. "Basic256Sha256".
    QStringList securityPolicies;
    /// \brief Distinct message security mode names of the enabled endpoints, e.g. "Sign & Encrypt".
    QStringList securityModes;
    /// \brief Reason the endpoints cannot be served; empty when they can.
    QString error;

    ///
    /// \brief Tells whether the endpoints can be served.
    /// \return True when error is empty.
    ///
    bool ok() const { return error.isEmpty(); }
};

namespace ListenerSettingsBuilder {

///
/// \brief Returns the security policy names an endpoint can use.
/// \return Policy names in display order, starting with "None".
///
QStringList securityPolicyNames();

///
/// \brief Returns the message security mode names an endpoint can use.
/// \return "None", "Sign" and "Sign & Encrypt".
///
QStringList securityModeNames();

///
/// \brief Merges the enabled endpoints into listener settings.
/// \param endpoints Configured endpoints.
/// \return Settings, or an error when no endpoint is enabled, enabled endpoints use different
///         ports, a port is out of range, a policy or mode is unknown, or a username endpoint
///         has no user name.
///
ListenerSettings build(const QList<EndpointInfo> &endpoints);

}
