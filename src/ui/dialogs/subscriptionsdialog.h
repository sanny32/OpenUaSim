// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file subscriptionsdialog.h
/// \brief Declares the dialog that lists the subscriptions of a session.
///

#pragma once

#include <QDialog>

#include "data/sessioninfo.h"

namespace Ui {
class SubscriptionsDialog;
}

class SubscriptionsModel;

///
/// \brief Lists the subscriptions a session held when the dialog opened.
///
class SubscriptionsDialog : public QDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the dialog for a session.
    /// \param session Session whose subscriptions are listed.
    /// \param parent Parent widget.
    ///
    explicit SubscriptionsDialog(const SessionInfo &session, QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog.
    ///
    ~SubscriptionsDialog() override;

    ///
    /// \brief Returns the subscriptions table model.
    /// \return Model of the listed subscriptions.
    ///
    const SubscriptionsModel *subscriptionsModel() const;

private:
    Ui::SubscriptionsDialog *ui;
    SubscriptionsModel *_model;
};
