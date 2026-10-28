// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file usersdialog.h
/// \brief Declares the dialog that edits the project's user accounts.
///

#pragma once

#include "appbasedialog.h"
#include <QList>

#include "data/projectconfig.h"

namespace Ui {
class UsersDialog;
}

///
/// \brief Lists the user accounts in an editable table and adds or removes accounts.
///
class UsersDialog : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the dialog showing the current accounts.
    /// \param users Accounts to edit.
    /// \param parent Parent widget.
    ///
    explicit UsersDialog(const QList<UserAccount> &users, QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog.
    ///
    ~UsersDialog() override;

    ///
    /// \brief Returns the edited accounts.
    /// \return Accounts in table order with the user names trimmed.
    ///
    QList<UserAccount> users() const;

private:
    void appendRow(const UserAccount &user);
    void addUser();
    void removeUser();
    void validate();

    Ui::UsersDialog *ui;
};
