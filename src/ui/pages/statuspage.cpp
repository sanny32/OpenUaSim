// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file statuspage.cpp
/// \brief Implements the server status page.
///

#include "appicons.h"
#include "data/serverstatus.h"
#include "formatting.h"
#include "statuspage.h"
#include "ui_statuspage.h"
#include "viewsetup.h"

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
    ViewSetup::bindCopyButton(ui->copyEndpointButton, ui->primaryEndpointValue);
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
    _status = status;
    ui->serverStatusIndicator->setStatus(status.running ? StatusKind::Good : StatusKind::Inactive,
                                         status.running ? tr("Running") : tr("Stopped"));
    ui->primaryEndpointValue->setText(status.primaryEndpoint);
    ui->copyEndpointButton->setEnabled(!status.primaryEndpoint.isEmpty());
    ui->applicationUriValue->setText(status.applicationUri);
    ui->startTimeValue->setText(Formatting::dateTime(status.startTime));
    ui->uptimeValue->setText(Formatting::duration(status.uptimeSeconds));
    ui->securityModesValue->setText(status.securityModes.join(QStringLiteral(", ")));
    ui->totalNodesValue->setNum(status.totalNodes);
    ui->namespacesValue->setNum(status.namespaces);
    ui->clientsValue->setNum(status.clientsConnected);
    showDiscovery(status);
}

///
/// \brief Shows whether the running server is registered at the discovery server and why not; blank while stopped.
/// \param status Server summary.
///
void StatusPage::showDiscovery(const ServerStatus &status)
{
    const QString &url = status.discoveryServerUrl;
    if (!status.running)
        setDiscovery(StatusKind::None, QString(), QString());
    else if (url.isEmpty())
        setDiscovery(StatusKind::Inactive, QStringLiteral("x"), tr("Disabled"));
    else if (status.discoveryStatus.isEmpty())
        setDiscovery(StatusKind::Info, QStringLiteral("clock"), tr("Registering at %1").arg(url));
    else if (status.discoveryStatus == QLatin1String("Good"))
        setDiscovery(StatusKind::Good, QStringLiteral("check"), tr("Registered at %1").arg(url));
    else
        setDiscovery(StatusKind::Error, QStringLiteral("x"),
                     tr("Not registered at %1: %2").arg(url, status.discoveryStatus));
}

///
/// \brief Fills the discovery registration row.
/// \param kind Status giving the icon colour.
/// \param iconName Resource name of the icon; empty for none.
/// \param text Label text.
///
void StatusPage::setDiscovery(StatusKind kind, const QString &iconName, const QString &text)
{
    ui->discoveryIndicator->setIcon(iconName.isEmpty() ? QIcon() : AppIcons::statusIcon(iconName, kind));
    ui->discoveryIndicator->setStatus(kind, text);
}

///
/// \brief Retranslates the page and shows the last status again when the language changes.
/// \param event Change event.
///
void StatusPage::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        setStatus(_status);
    }
    QWidget::changeEvent(event);
}
