// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include "application.h"
#include "mainwindow.h"
#include "project/projectdocument.h"
#include "server/servercontroller.h"

///
/// \brief Application entry point.
/// \param argc Argument count passed by the runtime.
/// \param argv Argument vector passed by the runtime.
/// \return Application exit code.
///
int main(int argc, char *argv[])
{
    Application app(argc, argv);

    ServerController controller;
    ProjectDocument document(&controller);
    document.openLast();

    MainWindow window(&app.theme());
    window.setController(&controller);
    window.setDocument(&document);
    window.show();

    return app.exec();
}
