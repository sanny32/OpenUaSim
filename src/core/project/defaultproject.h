// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file defaultproject.h
/// \brief Declares the project a new document starts from.
///

#pragma once

#include "data/projectconfig.h"

namespace DefaultProject {

///
/// \brief Builds the sample factory-line project.
/// \return Project with an anonymous endpoint on port 48400 for every security policy and mode
///         pair, and a FactoryLine_01 folder holding a motor, a conveyor, a pump and a tank with
///         simulated variables.
///
ProjectConfig create();

}
