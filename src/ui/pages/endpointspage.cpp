// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file endpointspage.cpp
/// \brief Implements the endpoints page.
///

#include <QSortFilterProxyModel>

#include "endpointsmodel.h"
#include "endpointspage.h"
#include "ui_endpointspage.h"
#include "viewsetup.h"

namespace {

const QString kPasswordMask = QStringLiteral("password");

}

///
/// \brief Builds the page.
/// \param parent Parent widget.
///
EndpointsPage::EndpointsPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::EndpointsPage)
    , _endpointsModel(new EndpointsModel(this))
{
    ui->setupUi(this);
    ViewSetup::splitByStretch(ui->splitter);

    ViewSetup::configureTable(ui->endpointsView);
    _searchModel = ViewSetup::attachTableSearch(ui->endpointsView, _endpointsModel, ui->searchEdit);
    ViewSetup::fitColumnsWhenFilled(ui->endpointsView);

    connect(ui->endpointsView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &EndpointsPage::showEndpoint);
    connect(ui->usernamePasswordCheck, &QCheckBox::toggled,
            this, &EndpointsPage::updateCredentialFields);
}

///
/// \brief Destroys the page.
///
EndpointsPage::~EndpointsPage()
{
    delete ui;
}

///
/// \brief Shows the configured endpoints.
/// \param endpoints Endpoints to list.
///
void EndpointsPage::setEndpoints(const QList<EndpointInfo> &endpoints)
{
    _endpointsModel->setItems(endpoints);
    ViewSetup::selectRow(ui->endpointsView, 0);
}

///
/// \brief Returns the endpoints table model.
/// \return Model of the configured endpoints.
///
const EndpointsModel *EndpointsPage::endpointsModel() const
{
    return _endpointsModel;
}

///
/// \brief Fills the properties panel from the selected endpoint.
/// \param current Selected table index.
///
void EndpointsPage::showEndpoint(const QModelIndex &current)
{
    const QModelIndex source = _searchModel->mapToSource(current);
    ui->propertiesHost->setEnabled(source.isValid());
    if (!source.isValid())
        return;
    const EndpointInfo &endpoint = _endpointsModel->item(source.row());

    ui->nameEdit->setText(endpoint.name);
    ui->hostnameEdit->setText(endpoint.hostname);
    ui->portSpin->setValue(endpoint.port);
    ui->securityPolicyCombo->setCurrentText(endpoint.securityPolicy);
    ui->securityModeCombo->setCurrentText(endpoint.securityMode);
    ui->allowAnonymousCheck->setChecked(endpoint.allowAnonymous);
    ui->usernamePasswordCheck->setChecked(endpoint.usernamePassword);
    ui->usernameEdit->setText(endpoint.username);
    ui->passwordEdit->setText(kPasswordMask);
    ui->requireCertificateCheck->setChecked(endpoint.requireClientCertificate);
    ui->enabledCheck->setChecked(endpoint.enabled);
    updateCredentialFields();
}

///
/// \brief Enables the user name and password fields only when that token type is accepted.
///
void EndpointsPage::updateCredentialFields()
{
    const bool enabled = ui->usernamePasswordCheck->isChecked();
    ui->usernameEdit->setEnabled(enabled);
    ui->passwordEdit->setEnabled(enabled);
}
