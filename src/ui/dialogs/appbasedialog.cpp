// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file appbasedialog.cpp
/// \brief Implements shared modal window behavior.
///

#include "appbasedialog.h"

#ifdef Q_OS_MAC
#include "macwindowbuttons.h"
#endif

///
/// \brief Constructs a dialog owned by the given widget.
/// \param parent Owning widget.
///
AppBaseDialog::AppBaseDialog(QWidget *parent)
    : QDialog(parent)
{
}

///
/// \brief Opens asynchronously, using a standalone modal window on macOS.
///
void AppBaseDialog::open()
{
#ifdef Q_OS_MAC
    // QDialog::open() forces WindowModal, which Cocoa presents as a sheet.
    setWindowModality(Qt::ApplicationModal);
    show();
#else
    QDialog::open();
#endif
}

///
/// \brief Keeps only the close button in the macOS title bar.
/// \param event Show event being handled.
///
void AppBaseDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
#ifdef Q_OS_MAC
    MacWindowButtons::hideMinimizeAndZoomButtons(this);
#endif
}
