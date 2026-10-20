// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file mainstatusbarwidget.cpp
/// \brief Implements the content of the main window status bar.
///

#include "data/serverstatus.h"
#include "mainstatusbarwidget.h"
#include "ui_mainstatusbarwidget.h"

///
/// \brief Builds the status bar content.
/// \param parent Parent widget.
///
MainStatusBarWidget::MainStatusBarWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainStatusBarWidget)
{
    ui->setupUi(this);
    ui->serverIndicator->setColoredText(true);
}

///
/// \brief Destroys the widget.
///
MainStatusBarWidget::~MainStatusBarWidget()
{
    delete ui;
}

///
/// \brief Shows whether the server is running.
/// \param status Server summary.
///
void MainStatusBarWidget::setServerStatus(const ServerStatus &status)
{
    _running = status.running;
    showRunning();
}

///
/// \brief Shows the last known server state in the indicator.
///
void MainStatusBarWidget::showRunning()
{
    ui->serverIndicator->setStatus(_running ? StatusKind::Good : StatusKind::Inactive,
                                   _running ? tr("Server running") : tr("Server stopped"));
}

///
/// \brief Retranslates the widget when the language changes, keeping the server state and the note.
/// \param event Change event.
///
void MainStatusBarWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        const QString trailing = trailingText();
        ui->retranslateUi(this);
        setTrailingText(trailing);
        showRunning();
    }
    QWidget::changeEvent(event);
}

///
/// \brief Sets the right-aligned note, e.g. the log entry count.
/// \param text Note text.
///
void MainStatusBarWidget::setTrailingText(const QString &text)
{
    ui->trailingLabel->setText(text);
}

///
/// \brief Returns the right-aligned note.
/// \return Note text.
///
QString MainStatusBarWidget::trailingText() const
{
    return ui->trailingLabel->text();
}
