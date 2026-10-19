// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file valuedialog.h
/// \brief Declares the viewer for a complete field value.
///

#pragma once

#include <QDialog>

namespace Ui {
class ValueDialog;
}

///
/// \brief Displays and copies a field value without truncation.
///
class ValueDialog : public QDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds a read-only viewer for a complete field value.
    /// \param title Field name used as the window title.
    /// \param value Original field value.
    /// \param parent Owning widget.
    ///
    ValueDialog(const QString &title, const QString &value, QWidget *parent = nullptr);

    ///
    /// \brief Destroys the viewer.
    ///
    ~ValueDialog() override;

private:
    void copyValue();

    Ui::ValueDialog *ui;
    QString _value;
};
