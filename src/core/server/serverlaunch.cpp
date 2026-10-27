// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file serverlaunch.cpp
/// \brief Implements the server state carried from one application run to the next.
///

#include "servercontroller.h"
#include "serverlaunch.h"
#include "settingsstore.h"

namespace {

const QString kRunningKey = QStringLiteral("server/running");
const QString kRestoreKey = QStringLiteral("server/restoreOnLaunch");

}

namespace ServerLaunch {

///
/// \brief Tells whether the server should start with the application.
/// \return The state the user left the server in last; false when no state is remembered.
///
bool shouldStart()
{
    return SettingsStore().value(kRunningKey, false).toBool();
}

///
/// \brief Remembers the state the user left the server in, for the next application run.
/// \param running True after a successful start, false after a stop or a failed start.
///
void remember(bool running)
{
    SettingsStore().setValue(kRunningKey, running);
}

///
/// \brief Tells whether the application restores the server state it was left in.
/// \return True unless the user turned restoring off.
///
bool isRestoreEnabled()
{
    return SettingsStore().value(kRestoreKey, true).toBool();
}

///
/// \brief Turns restoring the server state at application start on or off.
/// \param enabled False leaves the server stopped at start, whatever state it was left in.
///
void setRestoreEnabled(bool enabled)
{
    SettingsStore().setValue(kRestoreKey, enabled);
}

///
/// \brief Starts the server when restoring is enabled and the user left it running last; the remembered state is kept.
/// \param controller Controller serving the opened project.
/// \return True when the server was started.
///
bool restore(ServerController &controller)
{
    return isRestoreEnabled() && shouldStart() && controller.start();
}

}
