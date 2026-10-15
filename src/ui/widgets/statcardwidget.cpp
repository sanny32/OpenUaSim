// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file statcardwidget.cpp
/// \brief Implements the quick statistics card.
///

#include <QEvent>

#include "appcolors.h"
#include "appicons.h"
#include "statcardwidget.h"
#include "ui_statcardwidget.h"

namespace {

constexpr int kIconSize = 32;
constexpr int kChevronSize = 18;

}

///
/// \brief Builds an empty card.
/// \param parent Parent widget.
///
StatCardWidget::StatCardWidget(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::StatCardWidget)
{
    ui->setupUi(this);
    refreshPixmaps();
}

///
/// \brief Destroys the card.
///
StatCardWidget::~StatCardWidget()
{
    delete ui;
}

///
/// \brief Sets the card icon.
/// \param name Icon resource name.
/// \param color Colour the icon is drawn in.
///
void StatCardWidget::setIcon(const QString &name, const QColor &color)
{
    _iconName = name;
    _iconColor = color;
    refreshPixmaps();
}

///
/// \brief Sets the caption above the value.
/// \param caption Caption text.
///
void StatCardWidget::setCaption(const QString &caption)
{
    ui->captionLabel->setText(caption);
}

///
/// \brief Sets the value.
/// \param value Value text.
/// \param status Status colouring the value; StatusKind::None keeps the text colour.
///
void StatCardWidget::setValue(const QString &value, StatusKind status)
{
    _valueStatus = status;
    ui->valueLabel->setText(value);
    refreshPixmaps();
}

///
/// \brief Returns the value text.
/// \return Value text.
///
QString StatCardWidget::value() const
{
    return ui->valueLabel->text();
}

///
/// \brief Shows or hides the chevron hinting that the card leads to a page.
/// \param visible True to show the chevron.
///
void StatCardWidget::setChevronVisible(bool visible)
{
    ui->chevronLabel->setVisible(visible);
}

///
/// \brief Re-renders the theme-dependent pixmaps and colours after a palette change.
/// \param event Change event.
///
void StatCardWidget::changeEvent(QEvent *event)
{
    QFrame::changeEvent(event);
    if (event->type() == QEvent::PaletteChange)
        refreshPixmaps();
}

///
/// \brief Renders the icon, the chevron and the value colour for the active palette.
///
void StatCardWidget::refreshPixmaps()
{
    if (!_iconName.isEmpty())
        ui->iconLabel->setPixmap(AppIcons::icon(_iconName, _iconColor).pixmap(kIconSize));
    ui->chevronLabel->setPixmap(AppIcons::icon(QStringLiteral("chevron-right")).pixmap(kChevronSize));

    QPalette valuePalette = ui->valueLabel->palette();
    valuePalette.setColor(QPalette::WindowText, AppColors::statusText(_valueStatus));
    ui->valueLabel->setPalette(valuePalette);
}
