// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file valuedialog.cpp
/// \brief Implements the viewer for a complete field value.
///

#include <QApplication>
#include <QClipboard>
#include <QFontDatabase>

#include "appicons.h"
#include "ui_valuedialog.h"
#include "valuedialog.h"

///
/// \brief Builds a read-only viewer for a complete field value.
/// \param title Field name used as the window title.
/// \param value Original field value.
/// \param parent Owning widget.
///
ValueDialog::ValueDialog(const QString &title, const QString &value, QWidget *parent)
    : AppBaseDialog(parent), ui(new Ui::ValueDialog), _value(value)
{
    ui->setupUi(this);
    setWindowTitle(title);
    ui->valueEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    ui->valueEdit->setPlainText(value);
    ui->copyButton->setIcon(AppIcons::icon(QStringLiteral("copy")));
    connect(ui->copyButton, &QPushButton::clicked, this, &ValueDialog::copyValue);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

///
/// \brief Destroys the viewer.
///
ValueDialog::~ValueDialog()
{
    delete ui;
}

///
/// \brief Copies the original value, including whitespace, to the clipboard.
///
void ValueDialog::copyValue()
{
    QApplication::clipboard()->setText(_value);
}
