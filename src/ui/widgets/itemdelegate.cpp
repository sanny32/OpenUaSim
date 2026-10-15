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

constexpr int kMinimumRowHeight = 30;
constexpr int kMultiLinePadding = 12;

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
/// \brief Returns the cell size with the row height raised to the minimum and
///        multi-line cells given extra vertical padding.
/// \param option Style option of the cell.
/// \param index Cell index.
/// \return Cell size.
///
QSize ItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    const bool multiLine = index.data().toString().contains(QLatin1Char('\n'));
    size.setHeight(qMax(size.height() + (multiLine ? kMultiLinePadding : 0), kMinimumRowHeight));
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

    QIcon icon;
    const QVariant iconName = index.data(ItemRoles::IconName);
    if (iconName.isValid())
        icon = AppIcons::icon(iconName.toString());
    else
        icon = AppIcons::statusDot(ItemRoles::statusFrom(index.data(ItemRoles::Status)));

    if (icon.isNull())
        return;
    option->icon = icon;
    option->features |= QStyleOptionViewItem::HasDecoration;
    if (!option->decorationSize.isValid() || option->decorationSize.width() < 16)
        option->decorationSize = QSize(16, 16);
}
