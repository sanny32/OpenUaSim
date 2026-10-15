// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file mockdata.h
/// \brief Declares the sample server state shown until the UI is bound to a real server.
///

#pragma once

#include "data/serversnapshot.h"

namespace MockData {

///
/// \brief Builds a sample factory-line server state.
/// \return Snapshot with a running server, an address space of four devices, a small
///         type system, namespaces, endpoints, certificates, sessions and a log.
///
ServerSnapshot snapshot();

}
