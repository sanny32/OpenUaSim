// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file renamenodedialog.cpp
/// \brief Implements the node browse name editor.
///

#include <QPushButton>

#include "renamenodedialog.h"
#include "ui_renamenodedialog.h"

///
/// \brief Builds an editor with the current browse name selected.
/// \param browseName Current node browse name.
/// \param parent Owning widget.
///
RenameNodeDialog::RenameNodeDialog(const QString &browseName, QWidget *parent)
    : AppBaseDialog(parent), ui(new Ui::RenameNodeDialog)
{
    ui->setupUi(this);
    ui->nameLabel->setText(tr("New name of '%1':").arg(browseName));
    ui->nameEdit->setText(browseName);
    ui->nameEdit->selectAll();
    ui->nameEdit->setFocus();
    connect(ui->nameEdit, &QLineEdit::textChanged, this, &RenameNodeDialog::validate);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    validate();
}

///
/// \brief Destroys the editor.
///
RenameNodeDialog::~RenameNodeDialog()
{
    delete ui;
}

///
/// \brief Returns the entered browse name.
///
QString RenameNodeDialog::browseName() const
{
    return ui->nameEdit->text();
}

///
/// \brief Prevents accepting an empty input, matching QInputDialog.
///
void RenameNodeDialog::validate()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!browseName().isEmpty());
}
