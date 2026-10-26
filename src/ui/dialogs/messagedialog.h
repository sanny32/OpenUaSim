// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file messagedialog.h
/// \brief Declares the application message box: notices, confirmations and three-way questions.
///

#pragma once

#include <QDialog>
#include <QIcon>
#include <QStyle>

namespace Ui {
class MessageDialog;
}

///
/// \brief Shows a message above a quieter explanation, with the buttons in a footer band.
///
/// The filled action button accepts, Cancel rejects and the optional alternative button on the
/// left finishes with MessageDialog::Alternative.
///
class MessageDialog : public QDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Result of the alternative button, next to QDialog::Accepted and QDialog::Rejected.
    ///
    static constexpr int Alternative = 2;

    ///
    /// \brief Builds a notice with a single OK button.
    /// \param icon SP_MessageBoxInformation, SP_MessageBoxQuestion, SP_MessageBoxWarning or SP_MessageBoxCritical.
    /// \param title Window title.
    /// \param text Main message, shown emphasised.
    /// \param details Explanation, shown quieter; hidden when empty.
    /// \param parent Owning widget.
    ///
    MessageDialog(QStyle::StandardPixmap icon, const QString &title, const QString &text,
                  const QString &details = QString(), QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog.
    ///
    ~MessageDialog() override;

    ///
    /// \brief Builds a warning that asks before an action, with Cancel as the default button.
    /// \param title Window title.
    /// \param question Main question, shown emphasised.
    /// \param details Consequences of the action, shown quieter; hidden when empty.
    /// \param action Text of the button that confirms, e.g. "Delete".
    /// \param parent Owning widget.
    /// \return Dialog to open; the caller owns it.
    ///
    static MessageDialog *confirmation(const QString &title, const QString &question, const QString &details,
                                       const QString &action, QWidget *parent = nullptr);

    ///
    /// \brief Opens a warning without blocking the event loop; it deletes itself when closed.
    /// \param title Window title.
    /// \param text Message.
    /// \param parent Owning widget.
    ///
    static void warning(const QString &title, const QString &text, QWidget *parent);

    ///
    /// \brief Shows a third button on the left of the footer that finishes with Alternative.
    /// \param text Button text, e.g. "Don't Save".
    ///
    void setAlternative(const QString &text);

    ///
    /// \brief Makes the action button the default, for actions that are safe to run on Enter.
    ///
    void setActionDefault();

    ///
    /// \brief Replaces the icon.
    /// \param icon Icon drawn at 40 logical pixels.
    ///
    void setIcon(const QIcon &icon);

protected:
    ///
    /// \brief Recolours the explanation when the theme changes.
    /// \param event Change event.
    ///
    void changeEvent(QEvent *event) override;

    ///
    /// \brief Fits the window to its text before it appears.
    /// \param event Show event.
    ///
    void showEvent(QShowEvent *event) override;

private:
    void applyColors();
    void fitToContents();

    Ui::MessageDialog *ui;
    QIcon _icon;
};
