// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file serversettingsdialog.h
/// \brief Declares the dialog that edits the server description.
///

#pragma once

#include "appbasedialog.h"

#include "data/projectconfig.h"

namespace Ui {
class ServerSettingsDialog;
}

///
/// \brief Edits the application name and URI, session limit, history limits and discovery registration of the server.
///
class ServerSettingsDialog : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the dialog showing the current settings.
    /// \param settings Settings to edit.
    /// \param parent Parent widget.
    ///
    explicit ServerSettingsDialog(const ServerSettings &settings, QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog.
    ///
    ~ServerSettingsDialog() override;

    ///
    /// \brief Returns the edited settings.
    /// \return Settings with the name, URI and discovery server URL trimmed.
    ///
    ServerSettings settings() const;

private:
    void validate();

    Ui::ServerSettingsDialog *ui;
};
