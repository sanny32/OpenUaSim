// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file valuepreviewdelegate.h
/// \brief Declares the delegate for opening truncated field values.
///

#pragma once

#include "itemdelegate.h"

///
/// \brief Adds an expand button when a compact preview cannot show its complete value.
///
class ValuePreviewDelegate : public ItemDelegate
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the field value delegate.
    /// \param parent Owning object.
    ///
    explicit ValuePreviewDelegate(QObject *parent = nullptr);

    ///
    /// \brief Draws the preview and its expand button when needed.
    /// \param painter Target painter.
    /// \param option Cell geometry and state.
    /// \param index Field value index.
    ///
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

signals:
    ///
    /// \brief Requests a viewer for the complete field value.
    /// \param index Field value index.
    ///
    void expandRequested(const QModelIndex &index);

protected:
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option,
                     const QModelIndex &index) override;

private:
    bool needsExpansion(const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QRect buttonRect(const QStyleOptionViewItem &option) const;
};
