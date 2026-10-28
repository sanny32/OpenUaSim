// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file messagedialog.cpp
/// \brief Implements the application message box: notices, confirmations and three-way questions.
///

#include <QEvent>
#include <QLabel>

#include "appcolors.h"
#include "appicons.h"
#include "messagedialog.h"
#include "ui_messagedialog.h"

namespace {

constexpr int kIconSize = 40;
constexpr double kTextScale = 1.15;
constexpr int kMaximumWidth = 720;

///
/// \brief Returns the width a label needs to show its text on one line.
/// \param label Label to measure; a hidden one needs no width.
/// \return Width in pixels.
///
int singleLineWidth(const QLabel *label)
{
    if (label->isHidden())
        return 0;
    const QMargins margins = label->contentsMargins();
    return label->fontMetrics().horizontalAdvance(label->text()) + margins.left() + margins.right() + 2 * label->margin();
}

}

///
/// \brief Builds a notice with a single OK button.
/// \param icon SP_MessageBoxInformation, SP_MessageBoxQuestion, SP_MessageBoxWarning or SP_MessageBoxCritical.
/// \param title Window title.
/// \param text Main message, shown emphasised.
/// \param details Explanation, shown quieter; hidden when empty.
/// \param parent Owning widget.
///
MessageDialog::MessageDialog(QStyle::StandardPixmap icon, const QString &title, const QString &text,
                             const QString &details, QWidget *parent)
    : AppBaseDialog(parent), ui(new Ui::MessageDialog), _icon(AppIcons::messageIcon(icon))
{
    ui->setupUi(this);
    setWindowTitle(title);
    ui->contentArea->setBackgroundRole(QPalette::Base);
    QFont font = ui->textLabel->font();
    font.setWeight(QFont::DemiBold);
    font.setPointSizeF(font.pointSizeF() * kTextScale);
    ui->textLabel->setFont(font);
    ui->textLabel->setText(text);
    ui->detailsLabel->setText(details);
    ui->detailsLabel->setVisible(!details.isEmpty());
    ui->alternativeButton->hide();
    ui->cancelButton->hide();
    setActionDefault();
    applyColors();

    connect(ui->actionButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->alternativeButton, &QPushButton::clicked, this, [this] { done(Alternative); });
}

///
/// \brief Destroys the dialog.
///
MessageDialog::~MessageDialog()
{
    delete ui;
}

///
/// \brief Builds a warning that asks before an action, with Cancel as the default button.
/// \param title Window title.
/// \param question Main question, shown emphasised.
/// \param details Consequences of the action, shown quieter; hidden when empty.
/// \param action Text of the button that confirms, e.g. "Delete".
/// \param parent Owning widget.
/// \return Dialog to open; the caller owns it.
///
MessageDialog *MessageDialog::confirmation(const QString &title, const QString &question, const QString &details,
                                           const QString &action, QWidget *parent)
{
    auto *dialog = new MessageDialog(QStyle::SP_MessageBoxWarning, title, question, details, parent);
    dialog->ui->actionButton->setText(action);
    dialog->ui->cancelButton->show();
    // The action cannot be undone, so a stray Enter must not run it.
    dialog->ui->actionButton->setDefault(false);
    dialog->ui->cancelButton->setDefault(true);
    dialog->ui->cancelButton->setFocus();
    return dialog;
}

///
/// \brief Opens a warning without blocking the event loop; it deletes itself when closed.
/// \param title Window title.
/// \param text Message.
/// \param parent Owning widget.
///
void MessageDialog::warning(const QString &title, const QString &text, QWidget *parent)
{
    auto *dialog = new MessageDialog(QStyle::SP_MessageBoxWarning, title, text, QString(), parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->open();
}

///
/// \brief Shows a third button on the left of the footer that finishes with Alternative.
/// \param text Button text, e.g. "Don't Save".
///
void MessageDialog::setAlternative(const QString &text)
{
    ui->alternativeButton->setText(text);
    ui->alternativeButton->show();
}

///
/// \brief Turns the notice into a choice: renames the action button and shows the button that rejects.
/// \param action Text of the button that accepts, e.g. "Restart Now".
/// \param cancel Text of the button that rejects, e.g. "Later".
///
void MessageDialog::setChoice(const QString &action, const QString &cancel)
{
    ui->actionButton->setText(action);
    ui->cancelButton->setText(cancel);
    ui->cancelButton->show();
}

///
/// \brief Makes the action button the default, for actions that are safe to run on Enter.
///
void MessageDialog::setActionDefault()
{
    ui->cancelButton->setDefault(false);
    ui->actionButton->setDefault(true);
    ui->actionButton->setFocus();
}

///
/// \brief Replaces the icon.
/// \param icon Icon drawn at 40 logical pixels.
///
void MessageDialog::setIcon(const QIcon &icon)
{
    _icon = icon;
    applyColors();
}

///
/// \brief Recolours the explanation when the theme changes.
/// \param event Change event.
///
void MessageDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::PaletteChange)
        applyColors();
    AppBaseDialog::changeEvent(event);
}

///
/// \brief Fits the window to its text before it appears.
/// \param event Show event.
///
void MessageDialog::showEvent(QShowEvent *event)
{
    fitToContents();
    AppBaseDialog::showEvent(event);
}

///
/// \brief Widens the window until the text fits on one line, within the minimum and kMaximumWidth, and
///        gives it exactly the height its wrapped text needs at that width.
///
void MessageDialog::fitToContents()
{
    // A word-wrapped label otherwise sizes the window for a narrow width and leaves the extra height empty.
    const QMargins content = ui->contentLayout->contentsMargins();
    const int chrome = content.left() + content.right() + kIconSize + ui->contentLayout->spacing();
    const int text = qMax(singleLineWidth(ui->textLabel), singleLineWidth(ui->detailsLabel));
    const int width = qBound(qMax(minimumWidth(), ui->footer->sizeHint().width()), chrome + text, kMaximumWidth);
    layout()->activate();
    setFixedSize(width, layout()->totalHeightForWidth(width));
}

///
/// \brief Draws the icon and gives the explanation the secondary text colour of the current palette.
///
void MessageDialog::applyColors()
{
    ui->iconLabel->setPixmap(_icon.pixmap(QSize(kIconSize, kIconSize), devicePixelRatioF()));
    QPalette palette = ui->detailsLabel->palette();
    palette.setColor(QPalette::WindowText, AppColors::secondaryText(this->palette()));
    ui->detailsLabel->setPalette(palette);
}
