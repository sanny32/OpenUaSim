// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file elidedlabel.cpp
/// \brief Implements the shared single-line overflow behavior for labels.
///

#include <QHelpEvent>
#include <QPainter>
#include <QStyle>
#include <QToolTip>

#include "elidedlabel.h"
#include "texttooltip.h"

///
/// \brief Constructs an empty plain-text label.
/// \param parent Parent widget.
///
ElidedLabel::ElidedLabel(QWidget *parent)
    : QLabel(parent)
{
    setTextFormat(Qt::PlainText);
}

///
/// \brief Constructs a plain-text label with an initial value.
/// \param text Initial value.
/// \param parent Parent widget.
///
ElidedLabel::ElidedLabel(const QString &text, QWidget *parent)
    : ElidedLabel(parent)
{
    setText(text);
}

///
/// \brief Tells whether the single-line value exceeds the available text width.
/// \return True when the value needs an ellipsis.
///
bool ElidedLabel::isElided() const
{
    return !wordWrap() && fontMetrics().horizontalAdvance(text()) > textRect().width();
}

///
/// \brief Allows a single-line value to shrink to an ellipsis in a layout.
/// \return Minimum size preserving the label height.
///
QSize ElidedLabel::minimumSizeHint() const
{
    QSize size = QLabel::minimumSizeHint();
    if (!wordWrap())
        size.setWidth(qMin(size.width(), fontMetrics().horizontalAdvance(QChar(0x2026)) + 2 * margin()));
    return size;
}

///
/// \brief Shows the complete clipped value unless the label has an explicit tooltip.
/// \param event Widget event.
/// \return True when the event was handled.
///
bool ElidedLabel::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip && toolTip().isEmpty()) {
        if (isElided())
            TextToolTip::show(static_cast<QHelpEvent *>(event), this, text());
        else
            QToolTip::hideText();
        return true;
    }
    return QLabel::event(event);
}

///
/// \brief Elides plain values while preserving native painting during text selection.
/// \param event Paint event.
///
void ElidedLabel::paintEvent(QPaintEvent *event)
{
    if (!isElided() || hasSelectedText()) {
        QLabel::paintEvent(event);
        return;
    }
    QFrame::paintEvent(event);
    QPainter painter(this);
    const QRect rect = textRect();
    style()->drawItemText(&painter, rect, alignment() | Qt::TextSingleLine, palette(), isEnabled(),
                          fontMetrics().elidedText(text(), Qt::ElideRight, qMax(0, rect.width())),
                          foregroundRole());
}

///
/// \brief Reserves the label frame, margins and alignment-dependent indent.
/// \return Available text rectangle.
///
QRect ElidedLabel::textRect() const
{
    QRect rect = contentsRect().adjusted(margin(), margin(), -margin(), -margin());
    int padding = indent();
    if (padding < 0)
        padding = frameWidth() > 0 ? fontMetrics().horizontalAdvance(QLatin1Char('x')) / 2 : 0;
    const Qt::Alignment visual = QStyle::visualAlignment(layoutDirection(), alignment());
    if (visual.testFlag(Qt::AlignLeft))
        rect.adjust(padding, 0, 0, 0);
    else if (visual.testFlag(Qt::AlignRight))
        rect.adjust(0, 0, -padding, 0);
    return rect;
}
