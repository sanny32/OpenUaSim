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
/// \brief Shows the server state, its primary endpoint and the client count.
/// \param status Server summary.
///
void MainStatusBarWidget::setServerStatus(const ServerStatus &status)
{
    ui->serverIndicator->setStatus(status.running ? StatusKind::Good : StatusKind::Inactive,
                                   status.running ? tr("Server running") : tr("Server stopped"));
    ui->endpointLabel->setText(tr("Endpoint: %1").arg(status.primaryEndpoint));
    ui->clientsLabel->setText(tr("%n client(s) connected", nullptr, status.clientsConnected));
}

///
/// \brief Sets the right-aligned note, e.g. the node or log entry count.
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
