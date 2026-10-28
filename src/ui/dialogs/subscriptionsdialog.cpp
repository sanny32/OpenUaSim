// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file subscriptionsdialog.cpp
/// \brief Implements the dialog that lists the subscriptions of a session.
///

#include "subscriptionsdialog.h"
#include "subscriptionsmodel.h"
#include "ui_subscriptionsdialog.h"
#include "viewsetup.h"

///
/// \brief Builds the dialog for a session.
/// \param session Session whose subscriptions are listed.
/// \param parent Parent widget.
///
SubscriptionsDialog::SubscriptionsDialog(const SessionInfo &session, QWidget *parent)
    : AppBaseDialog(parent)
    , ui(new Ui::SubscriptionsDialog)
    , _model(new SubscriptionsModel(this))
{
    ui->setupUi(this);
    ui->sessionLabel->setText(tr("Subscriptions of session '%1' (%2)").arg(session.name, session.sessionId));
    ViewSetup::configureTable(ui->subscriptionsView);
    ui->subscriptionsView->setModel(_model);
    ViewSetup::fitColumnsWhenFilled(ui->subscriptionsView);
    _model->setItems(session.subscriptionList);
}

///
/// \brief Destroys the dialog.
///
SubscriptionsDialog::~SubscriptionsDialog()
{
    delete ui;
}

///
/// \brief Returns the subscriptions table model.
/// \return Model of the listed subscriptions.
///
const SubscriptionsModel *SubscriptionsDialog::subscriptionsModel() const
{
    return _model;
}
