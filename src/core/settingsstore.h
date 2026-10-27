// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file settingsstore.h
/// \brief Declares the QSettings binding to the product-wide settings branch.
///

#pragma once

#include <QSettings>

///
/// \brief QSettings bound to the single product branch, HKEY_CURRENT_USER\\Software\\OpenUaSim on Windows, instead of
///        the organization/application pair of a default QSettings. All persisted state goes through this class.
///
class SettingsStore : public QSettings
{
public:
    ///
    /// \brief Opens the settings of the product branch.
    ///
    SettingsStore();
};
