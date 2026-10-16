// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file listenersettings.cpp
/// \brief Implements how the configured endpoints collapse onto the single listener QUaServer provides.
///

#include <QCoreApplication>

#include "listenersettings.h"

namespace {

///
/// \brief Translates a message of the listener settings builder.
/// \param text Source text.
/// \return Translated text.
///
QString tr(const char *text)
{
    return QCoreApplication::translate("ListenerSettings", text);
}

///
/// \brief Builds settings that carry only an error.
/// \param error Reason the endpoints cannot be served.
/// \return Failed settings.
///
ListenerSettings failure(const QString &error)
{
    ListenerSettings settings;
    settings.error = error;
    return settings;
}

///
/// \brief Appends a name unless the list already holds it.
/// \param list List to extend.
/// \param name Name to add.
///
void appendUnique(QStringList &list, const QString &name)
{
    if (!list.contains(name))
        list.append(name);
}

}

namespace ListenerSettingsBuilder {

///
/// \brief Returns the security policy names an endpoint can use.
/// \return Policy names in display order, starting with "None".
///
QStringList securityPolicyNames()
{
    return { QStringLiteral("None"), QStringLiteral("Basic128Rsa15"), QStringLiteral("Basic256"),
             QStringLiteral("Basic256Sha256"), QStringLiteral("Aes128_Sha256_RsaOaep"),
             QStringLiteral("Aes256_Sha256_RsaPss") };
}

///
/// \brief Returns the message security mode names an endpoint can use.
/// \return "None", "Sign" and "Sign & Encrypt".
///
QStringList securityModeNames()
{
    return { QStringLiteral("None"), QStringLiteral("Sign"), QStringLiteral("Sign & Encrypt") };
}

///
/// \brief Merges the enabled endpoints into listener settings.
/// \param endpoints Configured endpoints.
/// \return Settings, or an error when no endpoint is enabled, enabled endpoints use different
///         ports, a port is out of range, a policy or mode is unknown, or a username endpoint
///         has no user name.
///
ListenerSettings build(const QList<EndpointInfo> &endpoints)
{
    ListenerSettings settings;
    bool first = true;
    for (const EndpointInfo &endpoint : endpoints) {
        if (!endpoint.enabled)
            continue;
        if (endpoint.port < 1 || endpoint.port > 65535)
            return failure(tr("Endpoint '%1' has an invalid port %2").arg(endpoint.name).arg(endpoint.port));
        if (first) {
            settings.hostname = endpoint.hostname;
            settings.port = quint16(endpoint.port);
            first = false;
        } else if (endpoint.port != settings.port) {
            return failure(tr("Enabled endpoints must share one port: '%1' uses %2, others use %3")
                               .arg(endpoint.name).arg(endpoint.port).arg(settings.port));
        }
        if (!securityPolicyNames().contains(endpoint.securityPolicy))
            return failure(tr("Endpoint '%1' has an unknown security policy '%2'")
                               .arg(endpoint.name, endpoint.securityPolicy));
        if (!securityModeNames().contains(endpoint.securityMode))
            return failure(tr("Endpoint '%1' has an unknown security mode '%2'")
                               .arg(endpoint.name, endpoint.securityMode));
        appendUnique(settings.securityPolicies, endpoint.securityPolicy);
        appendUnique(settings.securityModes, endpoint.securityMode);

        settings.anonymousAllowed = settings.anonymousAllowed || endpoint.allowAnonymous;
        if (endpoint.usernamePassword) {
            if (endpoint.username.isEmpty())
                return failure(tr("Endpoint '%1' accepts user names but has none set").arg(endpoint.name));
            settings.users.append({ endpoint.username, endpoint.password });
        }
    }
    if (first)
        return failure(tr("No endpoint is enabled"));
    return settings;
}

}
