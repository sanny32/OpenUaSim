// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file itemdelegate.cpp
/// \brief Implements the item delegate turning the custom item roles into decorations.
///

#include <QAbstractItemView>
#include <QApplication>
#include <QFontMetricsF>
#include <QHelpEvent>

#include "appicons.h"
#include "itemdelegate.h"
#include "itemroles.h"
#include "texttooltip.h"

namespace {

constexpr int kMinimumRowHeight = 27;
constexpr double kStatusDotScale = 0.75;

}

///
/// \brief Constructs the delegate.
/// \param parent Owning QObject, usually the view.
///
ItemDelegate::ItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

///
/// \brief Returns the cell size with the row height raised to the minimum.
/// \param option Style option of the cell.
/// \param index Cell index.
/// \return Cell size.
///
QSize ItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(qMax(size.height(), kMinimumRowHeight));
    return size;
}

///
/// \brief Shows the complete text as a tooltip while the cell elides it and the model gives no tooltip.
/// \param event Help event.
/// \param view View containing the cell.
/// \param option Cell geometry and state.
/// \param index Cell index.
/// \return Whether the event was handled.
///
bool ItemDelegate::helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option,
                             const QModelIndex &index)
{
    if (event->type() == QEvent::ToolTip && !index.data(Qt::ToolTipRole).isValid()) {
        QStyleOptionViewItem styled(option);
        initStyleOption(&styled, index);
        if (!styled.text.isEmpty() && textOverflows(styled)) {
            showTextTip(event, view, option.rect, styled.text);
            return true;
        }
    }
    return QStyledItemDelegate::helpEvent(event, view, option, index);
}

///
/// \brief Tells whether the text of a cell is wider than the room the style gives it.
/// \param styled Cell option already filled by initStyleOption().
/// \return True when the style elides the text.
///
bool ItemDelegate::textOverflows(const QStyleOptionViewItem &styled) const
{
    const QStyle *style = styled.widget ? styled.widget->style() : QApplication::style();
    const QRect text = style->subElementRect(QStyle::SE_ItemViewItemText, &styled, styled.widget);
    const int margin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, &styled, styled.widget) + 1;
    // The style elides by fractional width; a rounded advance misses overflows below half a pixel.
    return QFontMetricsF(styled.font).horizontalAdvance(styled.text) > text.width() - 2 * margin;
}

///
/// \brief Shows text as a tooltip over a cell.
/// \param event Help event that asked for the tooltip.
/// \param view View containing the cell.
/// \param cell Cell rectangle; the tooltip hides when the pointer leaves it.
/// \param text Plain text to show.
///
void ItemDelegate::showTextTip(const QHelpEvent *event, QAbstractItemView *view, const QRect &cell,
                               const QString &text)
{
    TextToolTip::show(event, view->viewport(), text, cell);
}

///
/// \brief Adds the icon or status dot the model describes through the custom roles and drops the
///        focus frame of the current cell.
/// \param option Option to fill.
/// \param index Cell index.
///
void ItemDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(option, index);
    // Rows are selected whole; a focus frame around the current cell alone reads as a stray selection.
    option->state &= ~QStyle::State_HasFocus;

    const QVariant iconName = index.data(ItemRoles::IconName);
    if (iconName.isValid()) {
        const QIcon icon = AppIcons::icon(iconName.toString());
        if (icon.isNull())
            return;
        option->icon = icon;
        option->features |= QStyleOptionViewItem::HasDecoration;
        if (!option->decorationSize.isValid() || option->decorationSize.width() < 16)
            option->decorationSize = QSize(16, 16);
        return;
    }

    const QIcon dot = AppIcons::statusDot(ItemRoles::statusFrom(index.data(ItemRoles::Status)));
    if (dot.isNull())
        return;
    // Status dots stay text-sized even in views with large icons.
    const int side = qRound(option->fontMetrics.height() * kStatusDotScale);
    option->icon = dot;
    option->features |= QStyleOptionViewItem::HasDecoration;
    option->decorationSize = QSize(side, side);
}
