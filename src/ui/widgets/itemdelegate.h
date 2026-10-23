// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file itemdelegate.h
/// \brief Declares the item delegate turning the custom item roles into decorations.
///

#pragma once

#include <QStyledItemDelegate>

///
/// \brief Item delegate drawing ItemRoles::IconName as a themed icon and
///        ItemRoles::Status as a coloured dot, with a uniform row height.
///
class ItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the delegate.
    /// \param parent Owning QObject, usually the view.
    ///
    explicit ItemDelegate(QObject *parent = nullptr);

    ///
    /// \brief Returns the cell size with the row height raised to the minimum.
    /// \param option Style option of the cell.
    /// \param index Cell index.
    /// \return Cell size.
    ///
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    ///
    /// \brief Shows the complete text as a tooltip while the cell elides it and the model gives no tooltip.
    /// \param event Help event.
    /// \param view View containing the cell.
    /// \param option Cell geometry and state.
    /// \param index Cell index.
    /// \return Whether the event was handled.
    ///
    bool helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option,
                   const QModelIndex &index) override;

protected:
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
    bool textOverflows(const QStyleOptionViewItem &styled) const;
    static void showTextTip(const QHelpEvent *event, QAbstractItemView *view, const QRect &cell, const QString &text);
};
