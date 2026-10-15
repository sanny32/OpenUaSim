// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file endpointspage.cpp
/// \brief Implements the endpoints page.
///

#include "endpointsmodel.h"
#include "endpointspage.h"
#include "itemroles.h"
#include "treebuilders.h"
#include "treemodel.h"
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
    , _listModel(new TreeModel(this))
    , _endpointsModel(new EndpointsModel(this))
{
    ui->setupUi(this);
    ui->endpointsList->setIconSize(QSize(24, 24));

    ViewSetup::installDelegate(ui->endpointsList);
    ViewSetup::attachSearch(ui->endpointsList, _listModel, ui->searchEdit);
    ViewSetup::configureTable(ui->endpointsView);
    ui->endpointsView->setModel(_endpointsModel);

    connect(ui->endpointsList->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &EndpointsPage::showListItem);
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
    _listModel->setNodes(TreeBuilders::endpointList(endpoints));
    ViewSetup::applyInitialState(ui->endpointsList);
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
/// \brief Selects the table row of the endpoint picked in the list.
/// \param current Selected list index.
///
void EndpointsPage::showListItem(const QModelIndex &current)
{
    const QString url = current.data(ItemRoles::Id).toString();
    const auto &items = _endpointsModel->items();
    for (int row = 0; row < items.size(); ++row) {
        if (items.at(row).url() == url) {
            ViewSetup::selectRow(ui->endpointsView, row);
            return;
        }
    }
}

///
/// \brief Fills the properties panel from the selected endpoint and syncs the list.
/// \param current Selected table index.
///
void EndpointsPage::showEndpoint(const QModelIndex &current)
{
    ui->propertiesHost->setEnabled(current.isValid());
    if (!current.isValid())
        return;
    const EndpointInfo &endpoint = _endpointsModel->item(current.row());
    ViewSetup::selectById(ui->endpointsList, endpoint.url());

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
