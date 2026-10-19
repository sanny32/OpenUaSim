// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file serversettingsrules.h
/// \brief Declares the validation of the server description.
///

#pragma once

#include <QString>

#include "data/projectconfig.h"

namespace ServerSettingsRules {

///
/// \brief Returns why a server description cannot be served.
/// \param settings Application name, application URI and session limit.
/// \return Problem text, or empty when the name and URI are set and the session limit is within 1..65535.
///
QString problem(const ServerSettings &settings);

}
