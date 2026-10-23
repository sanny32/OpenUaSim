// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file testproject.h
/// \brief Declares the sample project tests start servers with.
///

#pragma once

#include "project/defaultproject.h"

namespace TestProject {

///
/// \brief Builds the sample project with its unsecured endpoint only, so tests start without a server certificate.
/// \return Default project whose single endpoint uses security policy and mode None.
///
inline ProjectConfig create()
{
    ProjectConfig project = DefaultProject::create();
    project.endpoints = { project.endpoints.first() };
    return project;
}

}
