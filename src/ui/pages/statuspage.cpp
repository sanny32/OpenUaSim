// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file statuspage.cpp
/// \brief Implements the server status page.
///

#include "data/serverstatus.h"
#include "formatting.h"
#include "statuspage.h"
#include "ui_statuspage.h"

///
/// \brief Builds the page.
/// \param parent Parent widget.
///
StatusPage::StatusPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::StatusPage)
{
    ui->setupUi(this);
    ui->serverStatusIndicator->setColoredText(true);
}

///
/// \brief Destroys the page.
///
StatusPage::~StatusPage()
{
    delete ui;
}

///
/// \brief Shows a server summary.
/// \param status Server summary.
///
void StatusPage::setStatus(const ServerStatus &status)
{
    ui->serverStatusIndicator->setStatus(status.running ? StatusKind::Good : StatusKind::Inactive,
                                         status.running ? tr("Running") : tr("Stopped"));
    ui->primaryEndpointValue->setText(status.primaryEndpoint);
    ui->applicationUriValue->setText(status.applicationUri);
    ui->startTimeValue->setText(Formatting::dateTime(status.startTime));
    ui->uptimeValue->setText(Formatting::duration(status.uptimeSeconds));
    ui->securityModesValue->setText(status.securityModes.join(QStringLiteral(", ")));
    ui->totalNodesValue->setNum(status.totalNodes);
    ui->namespacesValue->setNum(status.namespaces);
    ui->clientsValue->setNum(status.clientsConnected);
}
