// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file valuepreviewdelegate.cpp
/// \brief Implements expand buttons for truncated field values.
///

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOptionToolButton>

#include "appicons.h"
#include "valuepreviewdelegate.h"

///
/// \brief Builds the field value delegate.
/// \param parent Owning object.
///
ValuePreviewDelegate::ValuePreviewDelegate(QObject *parent)
    : ItemDelegate(parent)
{
}

///
/// \brief Draws the preview and its expand button when needed.
/// \param painter Target painter.
/// \param option Cell geometry and state.
/// \param index Field value index.
///
void ValuePreviewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
    if (!needsExpansion(option, index)) {
        ItemDelegate::paint(painter, option, index);
        return;
    }
    const QRect button = buttonRect(option);
    QStyleOptionViewItem preview(option);
    if (option.direction == Qt::RightToLeft)
        preview.rect.setLeft(button.right() + 2);
    else
        preview.rect.setRight(button.left() - 2);
    painter->fillRect(option.rect, option.state & QStyle::State_Selected
                                       ? option.palette.highlight() : option.palette.base());
    ItemDelegate::paint(painter, preview, index);
    QStyleOptionToolButton tool;
    tool.rect = button;
    tool.state = QStyle::State_Enabled | QStyle::State_AutoRaise;
    tool.palette = option.palette;
    tool.icon = AppIcons::icon(QStringLiteral("external-link"));
    tool.iconSize = QSize(16, 16);
    tool.subControls = QStyle::SC_ToolButton;
    const QStyle *style = option.widget ? option.widget->style() : QApplication::style();
    style->drawComplexControl(QStyle::CC_ToolButton, &tool, painter, option.widget);
}

///
/// \brief Opens a value only when its expand button receives a left click.
/// \param event Input event.
/// \param model Field model.
/// \param option Cell geometry and state.
/// \param index Field value index.
/// \return Whether the button handled the event.
///
bool ValuePreviewDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
                                     const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if ((event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease)
        && needsExpansion(option, index)) {
        const auto *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->button() == Qt::LeftButton && buttonRect(option).contains(mouse->position().toPoint())) {
            if (event->type() == QEvent::MouseButtonRelease)
                emit expandRequested(index);
            return true;
        }
    }
    return ItemDelegate::editorEvent(event, model, option, index);
}

///
/// \brief Detects hidden lines, whitespace or text exceeding the available cell width.
/// \param option Cell geometry and state.
/// \param index Field value index.
/// \return Whether the complete value needs a separate viewer.
///
bool ValuePreviewDelegate::needsExpansion(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.data(Qt::UserRole).isValid())
        return false;
    QStyleOptionViewItem styled(option);
    initStyleOption(&styled, index);
    const QString value = index.data(Qt::UserRole).toString();
    if (value.isEmpty())
        return false;
    const QStyle *style = option.widget ? option.widget->style() : QApplication::style();
    const QRect text = style->subElementRect(QStyle::SE_ItemViewItemText, &styled, option.widget);
    const int margin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, option.widget) + 1;
    return value != styled.text || styled.fontMetrics.horizontalAdvance(styled.text) > text.width() - 2 * margin;
}

///
/// \brief Places the expand button at the trailing edge of the cell.
/// \param option Cell geometry and layout direction.
/// \return Button rectangle in viewport coordinates.
///
QRect ValuePreviewDelegate::buttonRect(const QStyleOptionViewItem &option) const
{
    const int side = qMin(22, option.rect.height() - 4);
    const QRect button(option.rect.right() - side - 1, option.rect.center().y() - side / 2, side, side);
    return QStyle::visualRect(option.direction, option.rect, button);
}
