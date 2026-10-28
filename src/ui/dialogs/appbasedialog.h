// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file appbasedialog.h
/// \brief Declares shared modal window behavior.
///

#pragma once

#include <QDialog>

///
/// \brief Presents application dialogs as titled modal windows on macOS.
///
class AppBaseDialog : public QDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs a dialog owned by the given widget.
    /// \param parent Owning widget.
    ///
    explicit AppBaseDialog(QWidget *parent = nullptr);

public slots:
    ///
    /// \brief Opens asynchronously, using a standalone modal window on macOS.
    ///
    void open() override;

protected:
    void showEvent(QShowEvent *event) override;
};
