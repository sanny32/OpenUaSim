// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file confirmdialog.cpp
/// \brief Implements the confirmation dialog of actions that cannot be undone.
///

#include <QEvent>

#include "appcolors.h"
#include "appicons.h"
#include "confirmdialog.h"
#include "ui_confirmdialog.h"

namespace {

constexpr int kIconSize = 40;
constexpr double kQuestionScale = 1.15;

}

///
/// \brief Builds the dialog with a warning icon and Cancel as the default button.
/// \param title Window title.
/// \param question Main question, shown emphasised.
/// \param details Consequences of the action, shown quieter; hidden when empty.
/// \param action Text of the button that confirms, e.g. "Delete".
/// \param parent Owning widget.
///
ConfirmDialog::ConfirmDialog(const QString &title, const QString &question, const QString &details,
                             const QString &action, QWidget *parent)
    : QDialog(parent), ui(new Ui::ConfirmDialog)
{
    ui->setupUi(this);
    setWindowTitle(title);
    ui->contentArea->setBackgroundRole(QPalette::Base);
    QFont font = ui->questionLabel->font();
    font.setWeight(QFont::DemiBold);
    font.setPointSizeF(font.pointSizeF() * kQuestionScale);
    ui->questionLabel->setFont(font);
    ui->questionLabel->setText(question);
    ui->detailsLabel->setText(details);
    ui->detailsLabel->setVisible(!details.isEmpty());
    ui->actionButton->setText(action);
    // A destructive action must not run on a stray Enter.
    ui->cancelButton->setDefault(true);
    ui->cancelButton->setFocus();
    applyColors();

    connect(ui->actionButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

///
/// \brief Destroys the dialog.
///
ConfirmDialog::~ConfirmDialog()
{
    delete ui;
}

///
/// \brief Replaces the warning icon.
/// \param icon SP_MessageBoxInformation, SP_MessageBoxQuestion, SP_MessageBoxWarning or SP_MessageBoxCritical.
///
void ConfirmDialog::setIcon(QStyle::StandardPixmap icon)
{
    _icon = icon;
    applyColors();
}

///
/// \brief Recolours the explanation when the theme changes.
/// \param event Change event.
///
void ConfirmDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::PaletteChange)
        applyColors();
    QDialog::changeEvent(event);
}

///
/// \brief Draws the icon and gives the explanation the secondary text colour of the current palette.
///
void ConfirmDialog::applyColors()
{
    ui->iconLabel->setPixmap(AppIcons::messageIcon(_icon).pixmap(QSize(kIconSize, kIconSize), devicePixelRatioF()));
    QPalette palette = ui->detailsLabel->palette();
    palette.setColor(QPalette::WindowText, AppColors::secondaryText(this->palette()));
    ui->detailsLabel->setPalette(palette);
}
