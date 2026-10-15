// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file application.cpp
/// \brief Implements the application class.
///

#include <QIcon>

#include "application.h"
#include "appstyle.h"

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

    // AppStyle draws on top of Fusion, which renders entirely from the palette: that is what
    // makes both themes possible without style sheets.
    setStyle(new AppStyle);
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
