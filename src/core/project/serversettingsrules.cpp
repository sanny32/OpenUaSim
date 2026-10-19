// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file serversettingsrules.cpp
/// \brief Implements the validation of the server description.
///

#include <QCoreApplication>

#include "serversettingsrules.h"

namespace ServerSettingsRules {

///
/// \brief Returns why a server description cannot be served.
/// \param settings Application name, application URI and session limit.
/// \return Problem text, or empty when the name and URI are set and the session limit is within 1..65535.
///
QString problem(const ServerSettings &settings)
{
    if (settings.applicationName.trimmed().isEmpty())
        return QCoreApplication::translate("ServerSettingsRules", "The application name is empty");
    if (settings.applicationUri.trimmed().isEmpty())
        return QCoreApplication::translate("ServerSettingsRules", "The application URI is empty");
    if (settings.maxSessions < 1 || settings.maxSessions > kMaxSessionLimit)
        return QCoreApplication::translate("ServerSettingsRules", "The session limit must be between 1 and %1")
            .arg(kMaxSessionLimit);
    return {};
}

}
