// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file recentdirectories.h
/// \brief Declares the file dialog directories carried from one application run to the next.
///

#pragma once

#include <QString>

namespace RecentDirectories {

///
/// \brief Kind of file a dialog picks; each kind keeps its own directory.
///
enum class Purpose {
    Project,
    Model
};

///
/// \brief Returns the directory a dialog of the purpose opened a file from last.
/// \param purpose Kind of file the dialog picks.
/// \return Absolute directory; empty when none is remembered or it no longer exists.
///
QString directory(Purpose purpose);

///
/// \brief Remembers the directory of a file picked in a dialog, for this and the next application runs.
/// \param purpose Kind of file the dialog picks.
/// \param fileName Picked file; ignored when empty.
///
void remember(Purpose purpose, const QString &fileName);

}
