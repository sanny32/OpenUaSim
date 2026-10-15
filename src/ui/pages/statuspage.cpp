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
    // Both panels expand, so QSplitter would ignore their stretch factors; the sizes act as a ratio.
    ui->splitter->setSizes({ 1930, 1000 });
    setupCards();
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
    const StatusKind runState = status.running ? StatusKind::Good : StatusKind::Inactive;
    const QString runText = status.running ? tr("Running") : tr("Stopped");
    const QString securityModes = status.securityModes.join(QStringLiteral(", "));

    ui->serverStatusIndicator->setStatus(runState, runText);
    ui->primaryEndpointValue->setText(status.primaryEndpoint);
    ui->httpsIndicator->setStatus(status.httpsEnabled ? StatusKind::Good : StatusKind::Inactive,
                                  status.httpsEnabled ? tr("Enabled") : tr("Disabled"));
    ui->applicationUriValue->setText(status.applicationUri);
    ui->startTimeValue->setText(Formatting::dateTime(status.startTime));
    ui->uptimeValue->setText(Formatting::duration(status.uptimeSeconds));
    ui->securityModesValue->setText(securityModes);
    ui->totalNodesValue->setNum(status.totalNodes);
    ui->namespacesValue->setNum(status.namespaces);
    ui->clientsValue->setNum(status.clientsConnected);

    ui->serverStatusCard->setValue(runText, runState);
    ui->clientsCard->setValue(QString::number(status.clientsConnected));
    ui->nodesCard->setValue(QString::number(status.totalNodes));
    ui->namespacesCard->setValue(QString::number(status.namespaces));
    ui->securityCard->setValue(securityModes);
}

///
/// \brief Gives each statistics card its icon and caption.
///
void StatusPage::setupCards()
{
    ui->serverStatusCard->setIcon(QStringLiteral("server"), QColor(0x22c55e));
    ui->serverStatusCard->setCaption(tr("Server Status"));
    ui->serverStatusCard->setChevronVisible(false);

    ui->clientsCard->setIcon(QStringLiteral("users-fill"), QColor(0x3b82f6));
    ui->clientsCard->setCaption(tr("Clients Connected"));

    ui->nodesCard->setIcon(QStringLiteral("nodes"), QColor(0xa78bfa));
    ui->nodesCard->setCaption(tr("Total Nodes"));

    ui->namespacesCard->setIcon(QStringLiteral("database-fill"), QColor(0xf5b82e));
    ui->namespacesCard->setCaption(tr("Namespaces"));

    ui->securityCard->setIcon(QStringLiteral("lock-fill"), QColor(0x3b82f6));
    ui->securityCard->setCaption(tr("Security Modes"));
}
