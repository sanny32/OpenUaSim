// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file elidedlineedit.cpp
/// \brief Implements the line edit that elides text it cannot fit.
///

#include <QHelpEvent>
#include <QPainter>
#include <QStyleOptionFrame>
#include <QToolTip>
#include <QToolButton>

#include "elidedlineedit.h"
#include "texttooltip.h"

namespace {

// Matches the private text margins of QLineEdit, so elided and focused text line up.
constexpr int kHorizontalMargin = 2;
constexpr int kVerticalMargin = 1;

}

///
/// \brief Constructs an empty line edit.
/// \param parent Parent widget.
///
ElidedLineEdit::ElidedLineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    connect(this, &QLineEdit::textChanged, this, &ElidedLineEdit::showStart);
}

///
/// \brief Tells whether the text is wider than the space shown for it.
/// \return True when the text is clipped.
///
bool ElidedLineEdit::isElided() const
{
    return fontMetrics().horizontalAdvance(displayText()) > textRect().width();
}

///
/// \brief Shows the full text as a tooltip while it is clipped.
/// \param event Widget event.
/// \return True when the event was handled.
///
bool ElidedLineEdit::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip && toolTip().isEmpty()) {
        const auto *help = static_cast<QHelpEvent *>(event);
        if (isElided())
            TextToolTip::show(help, this, displayText());
        else
            QToolTip::hideText();
        return true;
    }
    return QLineEdit::event(event);
}

///
/// \brief Paints the text elided while unfocused; editing paints as a plain line edit.
/// \param event Paint event.
///
void ElidedLineEdit::paintEvent(QPaintEvent *event)
{
    if (hasFocus() || !isElided()) {
        QLineEdit::paintEvent(event);
        return;
    }

    QPainter painter(this);
    QStyleOptionFrame panel;
    initStyleOption(&panel);
    style()->drawPrimitive(QStyle::PE_PanelLineEdit, &panel, &painter, this);

    const QRect rect = textRect();
    painter.setPen(palette().color(QPalette::Text));
    painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter,
                     fontMetrics().elidedText(displayText(), Qt::ElideRight, rect.width()));
}

///
/// \brief Returns the area the text is drawn in.
/// \return Contents rectangle without text margins or embedded action buttons.
///
QRect ElidedLineEdit::textRect() const
{
    QStyleOptionFrame panel;
    initStyleOption(&panel);
    const QRect contents = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
    QRect rect = contents.marginsRemoved(textMargins())
        .adjusted(kHorizontalMargin, kVerticalMargin, -kHorizontalMargin, -kVerticalMargin);
    const auto buttons = findChildren<QToolButton *>(QString(), Qt::FindDirectChildrenOnly);
    for (const QToolButton *button : buttons) {
        if (!button->isVisibleTo(this))
            continue;
        const QRect geometry = button->geometry();
        if (geometry.center().x() < contents.center().x())
            rect.setLeft(qMax(rect.left(), geometry.right() + 1 + kHorizontalMargin));
        else
            rect.setRight(qMin(rect.right(), geometry.left() - 1 - kHorizontalMargin));
    }
    return rect;
}

///
/// \brief Keeps newly displayed read-only values scrolled to their beginning.
///
void ElidedLineEdit::showStart()
{
    if (isReadOnly() && !hasFocus())
        setCursorPosition(0);
}
