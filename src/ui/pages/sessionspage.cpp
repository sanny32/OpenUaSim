// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file sessionspage.cpp
/// \brief Implements the client sessions page.
///

#include <QSortFilterProxyModel>

#include "appicons.h"
#include "formatting.h"
#include "sessionsmodel.h"
#include "sessionspage.h"
#include "ui_sessionspage.h"
#include "viewsetup.h"

namespace {

///
/// \brief Entries of the state filter combo box, in item order.
///
enum StateFilter { AllSessions, ActiveSessions, ClosedSessions };

}

///
/// \brief Builds the page.
/// \param parent Parent widget.
///
SessionsPage::SessionsPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SessionsPage)
    , _sessionsModel(new SessionsModel(this))
    , _stateModel(new QSortFilterProxyModel(this))
{
    ui->setupUi(this);
    ViewSetup::splitByStretch(ui->splitter);
    ViewSetup::bindCopyButton(ui->copySessionIdButton, ui->sessionIdEdit);
    ui->showSubscriptionsButton->setIcon(AppIcons::icon(QStringLiteral("list")));

    _stateModel->setSourceModel(_sessionsModel);
    _stateModel->setFilterKeyColumn(SessionsModel::ColState);
    ViewSetup::configureTable(ui->sessionsView);
    _searchModel = ViewSetup::attachTableSearch(ui->sessionsView, _stateModel, ui->searchEdit);
    ViewSetup::fitColumnsWhenFilled(ui->sessionsView);

    connect(ui->sessionsView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &SessionsPage::showSession);
    connect(ui->stateFilterCombo, &QComboBox::currentIndexChanged,
            this, &SessionsPage::applyStateFilter);
    connect(ui->showSubscriptionsButton, &QToolButton::clicked, this, &SessionsPage::requestSubscriptions);
    ui->showSubscriptionsButton->setEnabled(false);
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
    ViewSetup::selectRow(ui->sessionsView, 0);
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
/// \brief Limits the table to all, active or closed sessions and selects the first one shown.
/// \param filter Index of the state filter combo box entry.
///
void SessionsPage::applyStateFilter(int filter)
{
    QString state;
    if (filter == ActiveSessions)
        state = SessionsModel::stateText(true);
    else if (filter == ClosedSessions)
        state = SessionsModel::stateText(false);
    _stateModel->setFilterFixedString(state);
    ViewSetup::selectRow(ui->sessionsView, 0);
}

///
/// \brief Retranslates the page when the language changes, keeping the shown session.
/// \param event Change event.
///
void SessionsPage::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        showSession(ui->sessionsView->currentIndex());
        _sessionsModel->retranslate();
    }
    QWidget::changeEvent(event);
}

///
/// \brief Fills the properties panel from the selected session.
/// \param current Selected table index.
///
void SessionsPage::showSession(const QModelIndex &current)
{
    const SessionInfo *session = sessionAt(current);
    ui->propertiesHost->setEnabled(session != nullptr);
    ui->showSubscriptionsButton->setEnabled(session && !session->subscriptionList.isEmpty());
    if (session)
        showSessionDetails(*session);
}

///
/// \brief Returns the session behind a table row.
/// \param index Index of the filtered table.
/// \return Session, or null for an invalid index.
///
const SessionInfo *SessionsPage::sessionAt(const QModelIndex &index) const
{
    const QModelIndex source = _stateModel->mapToSource(_searchModel->mapToSource(index));
    return source.isValid() ? &_sessionsModel->item(source.row()) : nullptr;
}

///
/// \brief Asks to show the subscriptions of the selected session.
///
void SessionsPage::requestSubscriptions()
{
    if (const SessionInfo *session = sessionAt(ui->sessionsView->currentIndex()))
        emit subscriptionsRequested(*session);
}

///
/// \brief Fills the properties panel.
/// \param session Session to show.
///
void SessionsPage::showSessionDetails(const SessionInfo &session)
{
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
