// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file aboutdialog.h
/// \brief Declares the application information window.
///

#pragma once

#include "appbasedialog.h"

namespace Ui { class AboutDialog; }

class AboutDialog : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Presents product information and build diagnostics.
    /// \param parent Owning widget.
    ///
    explicit AboutDialog(QWidget *parent = nullptr);

    ///
    /// \brief Releases the generated form.
    ///
    ~AboutDialog() override;

protected:
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void fitToContents();
    void populateInformation();
    void applyColors();
    Ui::AboutDialog *ui;
};
