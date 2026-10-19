// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file usersdialog.cpp
/// \brief Implements the dialog that edits the project's user accounts.
///

#include <QPushButton>

#include "appicons.h"
#include "project/useraccountrules.h"
#include "ui_usersdialog.h"
#include "usersdialog.h"

namespace {

enum Column { ColUsername, ColPassword };

///
/// \brief Returns the text of a table cell.
/// \param item Cell; may be null for a cell never filled.
/// \return Cell text, or empty.
///
QString cellText(const QTableWidgetItem *item)
{
    return item ? item->text() : QString();
}

}

///
/// \brief Builds the dialog showing the current accounts.
/// \param users Accounts to edit.
/// \param parent Parent widget.
///
UsersDialog::UsersDialog(const QList<UserAccount> &users, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::UsersDialog)
{
    ui->setupUi(this);
    ui->addUserButton->setIcon(AppIcons::icon(QStringLiteral("add-circle")));
    ui->removeUserButton->setIcon(AppIcons::icon(QStringLiteral("trash")));
    for (const UserAccount &user : users)
        appendRow(user);

    connect(ui->usersTable, &QTableWidget::itemChanged, this, &UsersDialog::validate);
    connect(ui->addUserButton, &QPushButton::clicked, this, &UsersDialog::addUser);
    connect(ui->removeUserButton, &QPushButton::clicked, this, &UsersDialog::removeUser);
    validate();
}

///
/// \brief Destroys the dialog.
///
UsersDialog::~UsersDialog()
{
    delete ui;
}

///
/// \brief Returns the edited accounts.
/// \return Accounts in table order with the user names trimmed.
///
QList<UserAccount> UsersDialog::users() const
{
    QList<UserAccount> result;
    for (int row = 0; row < ui->usersTable->rowCount(); ++row)
        result.append({ cellText(ui->usersTable->item(row, ColUsername)).trimmed(),
                        cellText(ui->usersTable->item(row, ColPassword)) });
    return result;
}

///
/// \brief Adds a table row for an account.
/// \param user Account to show.
///
void UsersDialog::appendRow(const UserAccount &user)
{
    const int row = ui->usersTable->rowCount();
    ui->usersTable->insertRow(row);
    ui->usersTable->setItem(row, ColUsername, new QTableWidgetItem(user.username));
    ui->usersTable->setItem(row, ColPassword, new QTableWidgetItem(user.password));
}

///
/// \brief Appends an empty account and starts editing its user name.
///
void UsersDialog::addUser()
{
    appendRow({});
    QTableWidgetItem *name = ui->usersTable->item(ui->usersTable->rowCount() - 1, ColUsername);
    ui->usersTable->setCurrentItem(name);
    ui->usersTable->editItem(name);
    validate();
}

///
/// \brief Removes the selected account.
///
void UsersDialog::removeUser()
{
    const int row = ui->usersTable->currentRow();
    if (row >= 0)
        ui->usersTable->removeRow(row);
    validate();
}

///
/// \brief Shows why the accounts cannot be applied and allows OK only when they can.
///
void UsersDialog::validate()
{
    const QString problem = UserAccountRules::problem(users());
    ui->problemLabel->setText(problem);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(problem.isEmpty());
    ui->removeUserButton->setEnabled(ui->usersTable->rowCount() > 0);
}
