// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file endpointspage.cpp
/// \brief Implements the endpoints page.
///

#include <QSortFilterProxyModel>
#include <QStandardItemModel>

#include "endpointsmodel.h"
#include "endpointspage.h"
#include "server/listenersettings.h"
#include "ui_endpointspage.h"
#include "viewsetup.h"

namespace {

///
/// \brief Disables every combo entry except "None".
/// \param combo Combo filled from a QStandardItemModel, as QComboBox does by default.
/// \param toolTip Explanation shown on the disabled entries.
///
void disableAllButNone(QComboBox *combo, const QString &toolTip)
{
    auto *model = qobject_cast<QStandardItemModel *>(combo->model());
    for (int row = 0; model && row < model->rowCount(); ++row) {
        QStandardItem *item = model->item(row);
        if (item->text() == QLatin1String("None"))
            continue;
        item->setEnabled(false);
        item->setToolTip(toolTip);
    }
}

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
    restrictSecurityChoices();

    ViewSetup::configureTable(ui->endpointsView);
    _searchModel = ViewSetup::attachTableSearch(ui->endpointsView, _endpointsModel, ui->searchEdit);
    ViewSetup::fitColumnsWhenFilled(ui->endpointsView);

    connect(ui->endpointsView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &EndpointsPage::showEndpoint);
    connect(ui->securityPolicyCombo, &QComboBox::currentTextChanged,
            this, &EndpointsPage::matchModeToPolicy);
    connect(ui->securityModeCombo, &QComboBox::currentTextChanged,
            this, &EndpointsPage::matchPolicyToMode);
    connect(ui->usernamePasswordCheck, &QCheckBox::toggled,
            this, &EndpointsPage::updateCredentialFields);
    connect(ui->applyButton, &QPushButton::clicked, this, &EndpointsPage::applyEndpoint);
    connect(ui->revertButton, &QPushButton::clicked, this, &EndpointsPage::revertEndpoint);
    connect(ui->addEndpointButton, &QPushButton::clicked, this, &EndpointsPage::addEndpoint);
    connect(ui->removeEndpointButton, &QPushButton::clicked, this, &EndpointsPage::removeEndpoint);
}

///
/// \brief Destroys the page.
///
EndpointsPage::~EndpointsPage()
{
    delete ui;
}

///
/// \brief Shows the configured endpoints, keeping the selected row when it still exists.
/// \param endpoints Endpoints to list.
///
void EndpointsPage::setEndpoints(const QList<EndpointInfo> &endpoints)
{
    const int previous = qMax(0, ui->endpointsView->currentIndex().row());
    _endpointsModel->setItems(endpoints);
    ViewSetup::selectRow(ui->endpointsView, qMin(previous, int(endpoints.size()) - 1));
    ui->removeEndpointButton->setEnabled(!endpoints.isEmpty());
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
    {
        // Show the stored pair as is, even a mismatched one, instead of letting the combos correct each other.
        const QSignalBlocker policyBlocker(ui->securityPolicyCombo);
        const QSignalBlocker modeBlocker(ui->securityModeCombo);
        ui->securityPolicyCombo->setCurrentText(endpoint.securityPolicy);
        ui->securityModeCombo->setCurrentText(endpoint.securityMode);
    }
    ui->allowAnonymousCheck->setChecked(endpoint.allowAnonymous);
    ui->usernamePasswordCheck->setChecked(endpoint.usernamePassword);
    ui->usernameEdit->setText(endpoint.username);
    ui->passwordEdit->clear();
    ui->passwordEdit->setPlaceholderText(endpoint.password.isEmpty() ? QString() : tr("Unchanged"));
    ui->trustedClientsOnlyCheck->setChecked(endpoint.trustedClientsOnly);
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

///
/// \brief Switches the security mode to one the newly chosen policy can be served with.
/// \param policy Chosen security policy name.
///
void EndpointsPage::matchModeToPolicy(const QString &policy)
{
    if (!ListenerSettingsBuilder::securityPairs(policy, ui->securityModeCombo->currentText()))
        ui->securityModeCombo->setCurrentText(policy == QLatin1String("None") ? QStringLiteral("None")
                                                                              : QStringLiteral("Sign & Encrypt"));
}

///
/// \brief Switches the security policy to one the newly chosen mode can be served with.
/// \param mode Chosen message security mode name.
///
void EndpointsPage::matchPolicyToMode(const QString &mode)
{
    if (!ListenerSettingsBuilder::securityPairs(ui->securityPolicyCombo->currentText(), mode))
        ui->securityPolicyCombo->setCurrentText(mode == QLatin1String("None") ? QStringLiteral("None")
                                                                              : QStringLiteral("Basic256Sha256"));
}

///
/// \brief Offers only unsecured endpoints in a build without encryption.
///
void EndpointsPage::restrictSecurityChoices()
{
#ifndef OUASIM_HAVE_ENCRYPTION
    const QString reason = tr("This build has no encryption support");
    disableAllButNone(ui->securityPolicyCombo, reason);
    disableAllButNone(ui->securityModeCombo, reason);
#endif
}

///
/// \brief Returns the model row of the selected endpoint.
/// \return Row, or -1 when none is selected.
///
int EndpointsPage::currentRow() const
{
    const QModelIndex source = _searchModel->mapToSource(ui->endpointsView->currentIndex());
    return source.isValid() ? source.row() : -1;
}

///
/// \brief Reads the properties panel.
/// \param row Model row of the edited endpoint.
/// \return Endpoint with the edited fields; an empty password keeps the stored one.
///
EndpointInfo EndpointsPage::editedEndpoint(int row) const
{
    EndpointInfo endpoint = _endpointsModel->item(row);
    endpoint.name = ui->nameEdit->text().trimmed();
    endpoint.hostname = ui->hostnameEdit->text().trimmed();
    endpoint.port = ui->portSpin->value();
    endpoint.securityPolicy = ui->securityPolicyCombo->currentText();
    endpoint.securityMode = ui->securityModeCombo->currentText();
    endpoint.allowAnonymous = ui->allowAnonymousCheck->isChecked();
    endpoint.usernamePassword = ui->usernamePasswordCheck->isChecked();
    endpoint.username = ui->usernameEdit->text().trimmed();
    if (!ui->passwordEdit->text().isEmpty())
        endpoint.password = ui->passwordEdit->text();
    endpoint.trustedClientsOnly = ui->trustedClientsOnlyCheck->isChecked();
    endpoint.enabled = ui->enabledCheck->isChecked();
    return endpoint;
}

///
/// \brief Asks to store the edited properties of the selected endpoint.
///
void EndpointsPage::applyEndpoint()
{
    const int row = currentRow();
    if (row < 0)
        return;
    QList<EndpointInfo> endpoints = _endpointsModel->items();
    endpoints[row] = editedEndpoint(row);
    emit endpointsChangeRequested(endpoints);
}

///
/// \brief Discards edits by showing the stored properties again.
///
void EndpointsPage::revertEndpoint()
{
    showEndpoint(ui->endpointsView->currentIndex());
}

///
/// \brief Asks to add an unsecured endpoint on the port the others use.
///
void EndpointsPage::addEndpoint()
{
    QList<EndpointInfo> endpoints = _endpointsModel->items();
    EndpointInfo endpoint;
    endpoint.name = tr("New endpoint");
    endpoint.protocol = QStringLiteral("opc.tcp");
    endpoint.hostname = endpoints.isEmpty() ? QStringLiteral("localhost") : endpoints.first().hostname;
    endpoint.port = endpoints.isEmpty() ? endpoint.port : endpoints.first().port;
    endpoint.securityPolicy = QStringLiteral("None");
    endpoint.securityMode = QStringLiteral("None");
    endpoints.append(endpoint);
    emit endpointsChangeRequested(endpoints);
}

///
/// \brief Asks to remove the selected endpoint.
///
void EndpointsPage::removeEndpoint()
{
    const int row = currentRow();
    if (row < 0)
        return;
    QList<EndpointInfo> endpoints = _endpointsModel->items();
    endpoints.removeAt(row);
    emit endpointsChangeRequested(endpoints);
}
