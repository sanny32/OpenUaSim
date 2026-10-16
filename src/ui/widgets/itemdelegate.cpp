// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file itemdelegate.cpp
/// \brief Implements the item delegate turning the custom item roles into decorations.
///

#include "appicons.h"
#include "itemdelegate.h"
#include "itemroles.h"

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
/// \brief Adds the icon or status dot the model describes through the custom roles.
/// \param option Option to fill.
/// \param index Cell index.
///
void ItemDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(option, index);

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
