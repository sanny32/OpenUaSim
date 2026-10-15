// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include "application.h"
#include "mainwindow.h"
#include "mock/mockdata.h"

///
/// \brief Application entry point.
/// \param argc Argument count passed by the runtime.
/// \param argv Argument vector passed by the runtime.
/// \return Application exit code.
///
int main(int argc, char *argv[])
{
    Application app(argc, argv);

    MainWindow window(&app.theme());
    window.setSnapshot(MockData::snapshot());
    window.show();

    return app.exec();
}
