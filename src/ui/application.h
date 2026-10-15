// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file application.h
/// \brief Declares the application class.
///

#pragma once

#include <QApplication>

#include "apptheme.h"

///
/// \brief Application object: identity, the Fusion style, and the light/dark theme.
///
class Application : public QApplication
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the application, installs the style, and applies the saved theme.
    /// \param argc Argument count passed to main.
    /// \param argv Argument vector passed to main.
    ///
    Application(int &argc, char **argv);

    ///
    /// \brief Gives access to the theme manager.
    /// \return Application theme manager.
    ///
    AppTheme &theme();

private:
    AppTheme _theme;
};
