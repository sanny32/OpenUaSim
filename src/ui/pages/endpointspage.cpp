// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file endpointspage.cpp
/// \brief Implements the endpoints page.
///

#include <QMessageBox>
#include <QNetworkInterface>
#include <QSortFilterProxyModel>
#include <QSysInfo>
#include <QStandardItemModel>

#include <algorithm>

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

///
/// \brief Tells whether clients can reach the server through the interface's addresses.
/// \return False for interfaces that are down and for VPN or tunnel adapters.
///
bool isReachableInterface(const QNetworkInterface &networkInterface)
{
    const QNetworkInterface::InterfaceFlags flags = networkInterface.flags();
    // Wintun and tun adapters (e.g. AdGuard VPN, WireGuard) report Unknown type; only the missing MAC gives them away.
    const bool tunnelWithoutMac = networkInterface.hardwareAddress().isEmpty()
                                  && !flags.testFlag(QNetworkInterface::IsLoopBack);
    return flags.testFlag(QNetworkInterface::IsUp)
           && !flags.testFlag(QNetworkInterface::IsPointToPoint)
           && networkInterface.type() != QNetworkInterface::Virtual
           && networkInterface.type() != QNetworkInterface::Ppp
           && !tunnelWithoutMac;
}

///
/// \brief Offers local hosts and preserves hostnames from the project even when unavailable locally.
/// \param endpoints Configured endpoints whose hostnames must remain selectable.
/// \return Unique machine hostname, localhost, IPv4 addresses of reachable interfaces and configured hostnames.
///
QStringList hostnameChoices(const QList<EndpointInfo> &endpoints)
{
    QStringList hosts { QSysInfo::machineHostName(), QStringLiteral("localhost") };
    for (const QNetworkInterface &networkInterface : QNetworkInterface::allInterfaces()) {
        if (!isReachableInterface(networkInterface))
            continue;
        for (const QNetworkAddressEntry &entry : networkInterface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
                hosts.append(entry.ip().toString());
        }
    }
    for (const EndpointInfo &endpoint : endpoints)
        hosts.append(endpoint.hostname);
    hosts.removeAll(QString());
    hosts.removeDuplicates();
    return hosts;
}

using SecurityPair = QPair<QString, QString>;

///
/// \brief Returns the security pairs this build can serve.
/// \return Every pair OPC UA allows, or only ("None", "None") without encryption support.
///
QList<SecurityPair> offeredSecurityPairs()
{
#ifdef OUASIM_HAVE_ENCRYPTION
    return ListenerSettingsBuilder::allowedSecurityPairs();
#else
    return { { QStringLiteral("None"), QStringLiteral("None") } };
#endif
}

///
/// \brief Returns the security pairs of the endpoints.
/// \param endpoints Configured endpoints.
/// \return Pairs in endpoint order.
///
QList<SecurityPair> usedSecurityPairs(const QList<EndpointInfo> &endpoints)
{
    QList<SecurityPair> used;
    for (const EndpointInfo &endpoint : endpoints)
        used.append({ endpoint.securityPolicy, endpoint.securityMode });
    return used;
}

///
/// \brief Tells whether the endpoints already cover every security pair this build can serve.
/// \param endpoints Configured endpoints.
/// \return True when no pair is left for a new endpoint.
///
bool everySecurityPairUsed(const QList<EndpointInfo> &endpoints)
{
    const QList<SecurityPair> used = usedSecurityPairs(endpoints);
    const QList<SecurityPair> offered = offeredSecurityPairs();
    return std::all_of(offered.cbegin(), offered.cend(),
                       [&used](const SecurityPair &pair) { return used.contains(pair); });
}

///
/// \brief Tells whether moving an endpoint to a security pair would duplicate another endpoint.
/// \param endpoints Configured endpoints.
/// \param row Row of the edited endpoint.
/// \param pair Security pair chosen for it.
/// \return True when the pair differs from the stored one and another endpoint already uses it;
///         a duplicate already stored stays editable.
///
bool takesUsedSecurityPair(const QList<EndpointInfo> &endpoints, int row, const SecurityPair &pair)
{
    const QList<SecurityPair> used = usedSecurityPairs(endpoints);
    return used.at(row) != pair && used.contains(pair);
}

///
/// \brief Picks the security pair for a new endpoint: the first one after the last endpoint's pair
///        that no endpoint uses yet, wrapping around the list.
/// \param endpoints Configured endpoints.
/// \return Policy and mode; the pair right after the last endpoint's one when every pair is taken.
///
SecurityPair nextSecurityPair(const QList<EndpointInfo> &endpoints)
{
    const QList<SecurityPair> pairs = offeredSecurityPairs();
    const QList<SecurityPair> used = usedSecurityPairs(endpoints);

    const int last = used.isEmpty() ? -1 : int(pairs.indexOf(used.last()));
    for (int step = 1; step <= pairs.size(); ++step) {
        const SecurityPair &candidate = pairs.at((last + step) % pairs.size());
        if (!used.contains(candidate))
            return candidate;
    }
    return pairs.at((last + 1) % pairs.size());
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
    for (int column = EndpointsModel::ColProtocol; column < EndpointsModel::ColEnabled; ++column)
        ViewSetup::growColumnWithContents(ui->endpointsView, column);

    connect(ui->endpointsView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &EndpointsPage::showEndpoint);
    connect(ui->securityPolicyCombo, &QComboBox::currentTextChanged,
            this, &EndpointsPage::matchModeToPolicy);
    connect(ui->securityModeCombo, &QComboBox::currentTextChanged,
            this, &EndpointsPage::matchPolicyToMode);
    connect(ui->applyButton, &QPushButton::clicked, this, &EndpointsPage::applyEndpoint);
    connect(ui->revertButton, &QPushButton::clicked, this, &EndpointsPage::revertEndpoint);
    connect(ui->addEndpointButton, &QPushButton::clicked, this, &EndpointsPage::addEndpoint);
    connect(ui->removeEndpointButton, &QPushButton::clicked, this, &EndpointsPage::removeEndpoint);
    connect(ui->usersButton, &QPushButton::clicked, this, &EndpointsPage::usersEditRequested);
    ViewSetup::enableOnEdit(ui->propertiesHost, ui->applyButton);
    ui->applyButton->setEnabled(false);
}

///
/// \brief Destroys the page.
///
EndpointsPage::~EndpointsPage()
{
    delete ui;
}

///
/// \brief Shows endpoints and hostname choices, keeping the selected row when it still exists.
/// \param endpoints Endpoints to list.
///
void EndpointsPage::setEndpoints(const QList<EndpointInfo> &endpoints)
{
    const int previous = qMax(0, ui->endpointsView->currentIndex().row());
    ui->hostnameCombo->clear();
    ui->hostnameCombo->addItems(hostnameChoices(endpoints));
    _endpointsModel->setItems(endpoints);
    ViewSetup::selectRow(ui->endpointsView, qMin(previous, int(endpoints.size()) - 1));
    showEndpoint(ui->endpointsView->currentIndex());
    ui->removeEndpointButton->setEnabled(!endpoints.isEmpty());
    ui->addEndpointButton->setEnabled(!everySecurityPairUsed(endpoints));
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
    ui->propertiesActions->setEnabled(source.isValid());
    if (!source.isValid())
        return;
    const EndpointInfo &endpoint = _endpointsModel->item(source.row());

    ui->hostnameCombo->setCurrentText(endpoint.hostname);
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
    ui->trustedClientsOnlyCheck->setChecked(endpoint.trustedClientsOnly);
    ui->enabledCheck->setChecked(endpoint.enabled);
    ui->applyButton->setEnabled(false);
}

///
/// \brief Returns the security pair chosen in the properties panel.
/// \return Policy and mode.
///
QPair<QString, QString> EndpointsPage::chosenSecurityPair() const
{
    return { ui->securityPolicyCombo->currentText(), ui->securityModeCombo->currentText() };
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
/// \brief Retranslates the page when the language changes.
/// \param event Change event.
///
void EndpointsPage::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        restrictSecurityChoices();
        _endpointsModel->retranslate();
    }
    QWidget::changeEvent(event);
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
/// \return Endpoint with the edited fields.
///
EndpointInfo EndpointsPage::editedEndpoint(int row) const
{
    EndpointInfo endpoint = _endpointsModel->item(row);
    endpoint.hostname = ui->hostnameCombo->currentText().trimmed();
    endpoint.port = ui->portSpin->value();
    endpoint.securityPolicy = ui->securityPolicyCombo->currentText();
    endpoint.securityMode = ui->securityModeCombo->currentText();
    endpoint.allowAnonymous = ui->allowAnonymousCheck->isChecked();
    endpoint.usernamePassword = ui->usernamePasswordCheck->isChecked();
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
    const SecurityPair security = chosenSecurityPair();
    if (takesUsedSecurityPair(endpoints, row, security)) {
        auto *box = new QMessageBox(QMessageBox::Warning, tr("Endpoint Properties"),
                                    tr("Another endpoint already uses %1 with %2.")
                                        .arg(security.first, security.second),
                                    QMessageBox::Ok, this);
        box->setAttribute(Qt::WA_DeleteOnClose);
        connect(box, &QMessageBox::finished, this, &EndpointsPage::revertEndpoint);
        box->open();
        return;
    }
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
/// \brief Asks to add an endpoint on the port the others use, with the next unused security pair.
///
void EndpointsPage::addEndpoint()
{
    QList<EndpointInfo> endpoints = _endpointsModel->items();
    EndpointInfo endpoint;
    endpoint.protocol = QStringLiteral("opc.tcp");
    endpoint.hostname = endpoints.isEmpty() ? endpoint.hostname : endpoints.first().hostname;
    endpoint.port = endpoints.isEmpty() ? endpoint.port : endpoints.first().port;
    const SecurityPair security = nextSecurityPair(endpoints);
    endpoint.securityPolicy = security.first;
    endpoint.securityMode = security.second;
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
