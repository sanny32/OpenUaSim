// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file application.cpp
/// \brief Implements the application class.
///

#include <QIcon>

#include "application.h"

///
/// \brief Constructs the application, installs the style, and applies the saved theme.
/// \param argc Argument count passed to main.
/// \param argv Argument vector passed to main.
///
Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
{
    setOrganizationName(QStringLiteral(APP_ORGANIZATION_NAME));
    setApplicationName(QStringLiteral(APP_NAME));
    setApplicationVersion(QStringLiteral(APP_VERSION));
    setWindowIcon(QIcon(QStringLiteral(":/icons/app.svg")));

    // Fusion renders entirely from the palette, which is what makes both themes possible
    // without style sheets.
    setStyle(QStringLiteral("Fusion"));
    _theme.applySavedMode();
}

///
/// \brief Gives access to the theme manager.
/// \return Application theme manager.
///
AppTheme &Application::theme()
{
    return _theme;
}
