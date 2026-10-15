// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file sessionspage.cpp
/// \brief Implements the client sessions page.
///

#include "appicons.h"
#include "formatting.h"
#include "itemroles.h"
#include "sessionsmodel.h"
#include "sessionspage.h"
#include "treebuilders.h"
#include "treemodel.h"
#include "ui_sessionspage.h"
#include "viewsetup.h"

///
/// \brief Builds the page.
/// \param parent Parent widget.
///
SessionsPage::SessionsPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SessionsPage)
    , _treeModel(new TreeModel(this))
    , _sessionsModel(new SessionsModel(this))
{
    ui->setupUi(this);
    ViewSetup::bindCopyButton(ui->copySessionIdButton, ui->sessionIdEdit);
    ui->showSubscriptionsButton->setIcon(AppIcons::icon(QStringLiteral("list")));

    ViewSetup::installDelegate(ui->sessionsTree);
    ViewSetup::attachSearch(ui->sessionsTree, _treeModel, ui->searchEdit);
    ViewSetup::configureTable(ui->sessionsView);
    ui->sessionsView->setModel(_sessionsModel);

    connect(ui->sessionsTree->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &SessionsPage::showTreeNode);
    connect(ui->sessionsView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &SessionsPage::showSession);
}

///
/// \brief Destroys the page.
///
SessionsPage::~SessionsPage()
{
    delete ui;
}

///
/// \brief Shows client sessions.
/// \param sessions Active and closed sessions.
///
void SessionsPage::setSessions(const QList<SessionInfo> &sessions)
{
    _sessionsModel->setItems(sessions);
    _treeModel->setNodes(TreeBuilders::sessionTree(sessions));
    ViewSetup::applyInitialState(ui->sessionsTree);
    if (!sessions.isEmpty())
        ViewSetup::selectById(ui->sessionsTree, sessions.first().sessionId);
}

///
/// \brief Returns the sessions table model.
/// \return Model of the listed sessions.
///
const SessionsModel *SessionsPage::sessionsModel() const
{
    return _sessionsModel;
}

///
/// \brief Selects the table row of the session picked in the tree.
/// \param current Selected tree index.
///
void SessionsPage::showTreeNode(const QModelIndex &current)
{
    const int row = _sessionsModel->rowOf(current.data(ItemRoles::Id).toString());
    if (row >= 0)
        ViewSetup::selectRow(ui->sessionsView, row);
}

///
/// \brief Fills the properties panel from the selected session and syncs the tree.
/// \param current Selected table index.
///
void SessionsPage::showSession(const QModelIndex &current)
{
    ui->propertiesHost->setEnabled(current.isValid());
    if (!current.isValid())
        return;
    const SessionInfo &session = _sessionsModel->item(current.row());
    ViewSetup::selectById(ui->sessionsTree, session.sessionId);

    ui->sessionIdEdit->setText(session.sessionId);
    ui->sessionNameEdit->setText(session.name);
    ui->applicationUriEdit->setText(session.applicationUri);
    ui->endpointUrlEdit->setText(session.endpointUrl);
    ui->securityModeCombo->setCurrentText(session.securityMode);
    ui->userIdentityCombo->setCurrentText(session.userIdentity);
    ui->stateIndicator->setStatus(session.active ? StatusKind::Good : StatusKind::Inactive,
                                  SessionsModel::stateText(session.active));
    ui->createdEdit->setText(Formatting::dateTime(session.created));
    ui->lastActivityEdit->setText(Formatting::dateTime(session.lastActivity));
    ui->subscriptionsEdit->setText(QString::number(session.subscriptions));
}
