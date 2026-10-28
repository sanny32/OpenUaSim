// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file renamenodedialog.h
/// \brief Declares the node browse name editor.
///

#pragma once

#include "appbasedialog.h"

namespace Ui {
class RenameNodeDialog;
}

class RenameNodeDialog : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds an editor with the current browse name selected.
    /// \param browseName Current node browse name.
    /// \param parent Owning widget.
    ///
    explicit RenameNodeDialog(const QString &browseName, QWidget *parent = nullptr);

    ///
    /// \brief Destroys the editor.
    ///
    ~RenameNodeDialog() override;

    ///
    /// \brief Returns the entered browse name.
    ///
    QString browseName() const;

private:
    void validate();

    Ui::RenameNodeDialog *ui;
};
