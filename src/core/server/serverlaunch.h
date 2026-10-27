// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file serverlaunch.h
/// \brief Declares the server state carried from one application run to the next.
///

#pragma once

class ServerController;

namespace ServerLaunch {

///
/// \brief Tells whether the server should start with the application.
/// \return The state the user left the server in last; false when no state is remembered.
///
bool shouldStart();

///
/// \brief Remembers the state the user left the server in, for the next application run.
/// \param running True after a successful start, false after a stop or a failed start.
///
void remember(bool running);

///
/// \brief Tells whether the application restores the server state it was left in.
/// \return True unless the user turned restoring off.
///
bool isRestoreEnabled();

///
/// \brief Turns restoring the server state at application start on or off.
/// \param enabled False leaves the server stopped at start, whatever state it was left in.
///
void setRestoreEnabled(bool enabled);

///
/// \brief Starts the server when restoring is enabled and the user left it running last; the remembered state is kept.
/// \param controller Controller serving the opened project.
/// \return True when the server was started.
///
bool restore(ServerController &controller);

}
