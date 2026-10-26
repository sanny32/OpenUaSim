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
/// \param settings Application name, application URI, session limit, history limits and discovery registration.
/// \return Problem text, or empty when the name and URI are set, the session limit is within 1..65535, the
///         history keeps 1..kMaxHistoryValues values, their age limit is within 0..kMaxHistoryAgeMinutes and an
///         enabled discovery registration has an opc.tcp URL and an interval within the allowed range.
///
QString problem(const ServerSettings &settings);

}
