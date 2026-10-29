// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file opcuainfodialog.h
/// \brief Declares the OPC UA information dialog.
///

#pragma once

#include <QString>

#include "appbasedialog.h"

namespace Ui {
class OpcUaInfoDialog;
}

///
/// \brief Dialog that presents local OPC UA stack capabilities and reference links.
///
class OpcUaInfoDialog : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the OPC UA information dialog and fills its runtime fields.
    /// \param parent Parent widget.
    ///
    explicit OpcUaInfoDialog(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog and its generated UI.
    ///
    ~OpcUaInfoDialog() override;

    ///
    /// \brief Returns the plain-text diagnostic summary copied by the dialog.
    /// \return Current OPC UA information summary.
    ///
    QString copyText() const;

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void copyInfo();

private:
    void setupContent();
    void setupFonts();
    void setupLayout();
    void setupLogo();
    void setupLinks();

    Ui::OpcUaInfoDialog *ui;
};
