// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file settingsstore.cpp
/// \brief Implements the QSettings binding to the product-wide settings branch.
///

#include "settingsstore.h"

#ifdef Q_OS_WIN

namespace {

///
/// \brief Resolves where the product settings live for the current default format.
/// \return Registry branch of the product, or the INI file QSettings::setPath() redirects it to.
///
QString storagePath()
{
    // The organization-only constructor nests registry keys in an "OrganizationDefaults" subkey.
    if (QSettings::defaultFormat() == QSettings::NativeFormat)
        return QStringLiteral("HKEY_CURRENT_USER\\Software\\") + QStringLiteral(APP_NAME);
    return QSettings(QSettings::defaultFormat(), QSettings::UserScope, QStringLiteral(APP_NAME)).fileName();
}

}

///
/// \brief Opens the settings of the product branch.
///
SettingsStore::SettingsStore()
    : QSettings(storagePath(), QSettings::defaultFormat())
{
}

#else

///
/// \brief Opens the settings of the product branch.
///
SettingsStore::SettingsStore()
    // The format is explicit: the scope-and-organization overload alone always picks NativeFormat.
    : QSettings(QSettings::defaultFormat(), QSettings::UserScope, QStringLiteral(APP_NAME))
{
}

#endif
