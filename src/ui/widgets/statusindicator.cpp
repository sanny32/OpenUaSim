// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file statusindicator.cpp
/// \brief Implements the coloured dot-and-label status indicator.
///

#include <QPainter>

#include "appcolors.h"
#include "statusindicator.h"

namespace {

constexpr int kSpacing = 8;

}

///
/// \brief Constructs an indicator without status or text.
/// \param parent Parent widget.
///
StatusIndicator::StatusIndicator(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
}

///
/// \brief Sets the status and the label shown next to the dot.
/// \param kind Status selecting the dot colour.
/// \param text Label text.
///
void StatusIndicator::setStatus(StatusKind kind, const QString &text)
{
    _status = kind;
    _text = text;
    updateGeometry();
    update();
}

///
/// \brief Returns the shown status.
/// \return Status selecting the dot colour.
///
StatusKind StatusIndicator::status() const
{
    return _status;
}

///
/// \brief Returns the label text.
/// \return Label text.
///
QString StatusIndicator::text() const
{
    return _text;
}

///
/// \brief Chooses whether the label is drawn in the status colour.
/// \param colored True to colour the label, false for the palette text colour.
///
void StatusIndicator::setColoredText(bool colored)
{
    _coloredText = colored;
    update();
}

///
/// \brief Returns the size fitting the dot and the label.
/// \return Preferred size.
///
QSize StatusIndicator::sizeHint() const
{
    const QFontMetrics metrics = fontMetrics();
    const int textWidth = _text.isEmpty() ? 0 : kSpacing + metrics.horizontalAdvance(_text);
    return { dotDiameter() + textWidth, qMax(metrics.height(), dotDiameter()) };
}

///
/// \brief Returns the preferred size.
/// \return Same as sizeHint().
///
QSize StatusIndicator::minimumSizeHint() const
{
    return sizeHint();
}

///
/// \brief Draws the dot vertically centred, then the label.
/// \param event Unused.
///
void StatusIndicator::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int diameter = dotDiameter();
    const QRectF dot(0.5, (height() - diameter) / 2.0, diameter - 1, diameter - 1);
    painter.setPen(Qt::NoPen);
    painter.setBrush(AppColors::status(_status));
    painter.drawEllipse(dot);

    if (_text.isEmpty())
        return;
    painter.setPen(_coloredText ? AppColors::statusText(_status)
                                : palette().color(QPalette::WindowText));
    const QRect textRect = rect().adjusted(diameter + kSpacing, 0, 0, 0);
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, _text);
}

///
/// \brief Returns the dot size, proportional to the font.
/// \return Dot diameter in pixels.
///
int StatusIndicator::dotDiameter() const
{
    return qRound(fontMetrics().height() * 0.85);
}
