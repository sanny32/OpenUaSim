// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file recentdirectories.cpp
/// \brief Implements the file dialog directories carried from one application run to the next.
///

#include <QDir>
#include <QFileInfo>

#include "recentdirectories.h"
#include "settingsstore.h"

namespace {

///
/// \brief Returns the settings key of a purpose.
/// \param purpose Kind of file a dialog picks.
/// \return Key under the dialogs group.
///
QString key(RecentDirectories::Purpose purpose)
{
    switch (purpose) {
    case RecentDirectories::Purpose::Project:
        return QStringLiteral("dialogs/projectDirectory");
    case RecentDirectories::Purpose::Model:
        return QStringLiteral("dialogs/modelDirectory");
    }
    return QString();
}

}

namespace RecentDirectories {

///
/// \brief Returns the directory a dialog of the purpose opened a file from last.
/// \param purpose Kind of file the dialog picks.
/// \return Absolute directory; empty when none is remembered or it no longer exists.
///
QString directory(Purpose purpose)
{
    const QString stored = SettingsStore().value(key(purpose)).toString();
    return !stored.isEmpty() && QDir(stored).exists() ? stored : QString();
}

///
/// \brief Remembers the directory of a file picked in a dialog, for this and the next application runs.
/// \param purpose Kind of file the dialog picks.
/// \param fileName Picked file; ignored when empty.
///
void remember(Purpose purpose, const QString &fileName)
{
    if (!fileName.isEmpty())
        SettingsStore().setValue(key(purpose), QFileInfo(fileName).absolutePath());
}

}
