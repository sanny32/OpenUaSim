// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file listenersettings.cpp
/// \brief Implements how the configured endpoints collapse onto the single listener QUaServer provides.
///

#include <QCoreApplication>

#include <algorithm>

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
/// \brief Names an endpoint in messages; endpoints have no name of their own.
/// \param endpoint Endpoint to name.
/// \return Text such as "opc.tcp://localhost:48400 (None, None)".
///
QString label(const EndpointInfo &endpoint)
{
    return QStringLiteral("%1 (%2, %3)").arg(endpoint.url(), endpoint.securityPolicy, endpoint.securityMode);
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
/// \brief Tells whether a security policy and a message security mode form an endpoint OPC UA allows.
/// \param policy Name from securityPolicyNames().
/// \param mode Name from securityModeNames().
/// \return True when both are "None" or neither is.
///
bool securityPairs(const QString &policy, const QString &mode)
{
    const QString none = QStringLiteral("None");
    return (policy == none) == (mode == none);
}

///
/// \brief Tells whether an enabled endpoint needs a server certificate.
/// \param endpoints Configured endpoints.
/// \return True when an enabled endpoint uses a security policy other than "None".
///
bool usesSecurity(const QList<EndpointInfo> &endpoints)
{
    return std::any_of(endpoints.cbegin(), endpoints.cend(), [](const EndpointInfo &endpoint) {
        return endpoint.enabled && endpoint.securityPolicy != QLatin1String("None");
    });
}

///
/// \brief Merges the enabled endpoints into listener settings.
/// \param endpoints Configured endpoints.
/// \return Settings, or an error when no endpoint is enabled, enabled endpoints use different
///         ports, a port is out of range, a policy or mode is unknown, or a policy and mode do not
///         pair up.
///
ListenerSettings build(const QList<EndpointInfo> &endpoints)
{
    ListenerSettings settings;
    bool first = true;
    for (const EndpointInfo &endpoint : endpoints) {
        if (!endpoint.enabled)
            continue;
        if (endpoint.port < 1 || endpoint.port > 65535)
            return failure(tr("Endpoint %1 has an invalid port %2").arg(label(endpoint)).arg(endpoint.port));
        if (first) {
            settings.hostname = endpoint.hostname;
            settings.port = quint16(endpoint.port);
            first = false;
        } else if (endpoint.port != settings.port) {
            return failure(tr("Enabled endpoints must share one port: '%1' uses %2, others use %3")
                               .arg(label(endpoint)).arg(endpoint.port).arg(settings.port));
        }
        if (!securityPolicyNames().contains(endpoint.securityPolicy))
            return failure(tr("Endpoint %1 has an unknown security policy '%2'")
                               .arg(label(endpoint), endpoint.securityPolicy));
        if (!securityModeNames().contains(endpoint.securityMode))
            return failure(tr("Endpoint %1 has an unknown security mode '%2'")
                               .arg(label(endpoint), endpoint.securityMode));
        if (!securityPairs(endpoint.securityPolicy, endpoint.securityMode))
            return failure(tr("Endpoint %1 pairs security policy '%2' with mode '%3'; policy None needs "
                              "mode None and other policies need Sign or Sign & Encrypt")
                               .arg(label(endpoint), endpoint.securityPolicy, endpoint.securityMode));
        appendUnique(settings.securityPolicies, endpoint.securityPolicy);
        appendUnique(settings.securityModes, endpoint.securityMode);

        settings.anonymousAllowed = settings.anonymousAllowed || endpoint.allowAnonymous;
        settings.trustedClientsOnly = settings.trustedClientsOnly || endpoint.trustedClientsOnly;
        settings.usernameAllowed = settings.usernameAllowed || endpoint.usernamePassword;
    }
    if (first)
        return failure(tr("No endpoint is enabled"));
    return settings;
}

}
