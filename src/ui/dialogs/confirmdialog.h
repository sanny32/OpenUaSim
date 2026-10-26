// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file confirmdialog.h
/// \brief Declares the confirmation dialog of actions that cannot be undone.
///

#pragma once

#include <QDialog>
#include <QStyle>

namespace Ui {
class ConfirmDialog;
}

///
/// \brief Asks a question with an explanation below it; the action button accepts, Cancel rejects.
///
class ConfirmDialog : public QDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the dialog with a warning icon and Cancel as the default button.
    /// \param title Window title.
    /// \param question Main question, shown emphasised.
    /// \param details Consequences of the action, shown quieter; hidden when empty.
    /// \param action Text of the button that confirms, e.g. "Delete".
    /// \param parent Owning widget.
    ///
    ConfirmDialog(const QString &title, const QString &question, const QString &details, const QString &action,
                  QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog.
    ///
    ~ConfirmDialog() override;

    ///
    /// \brief Replaces the warning icon.
    /// \param icon SP_MessageBoxInformation, SP_MessageBoxQuestion, SP_MessageBoxWarning or SP_MessageBoxCritical.
    ///
    void setIcon(QStyle::StandardPixmap icon);

protected:
    ///
    /// \brief Recolours the explanation when the theme changes.
    /// \param event Change event.
    ///
    void changeEvent(QEvent *event) override;

private:
    void applyColors();

    Ui::ConfirmDialog *ui;
    QStyle::StandardPixmap _icon = QStyle::SP_MessageBoxWarning;
};
