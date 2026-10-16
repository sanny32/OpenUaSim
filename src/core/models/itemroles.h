// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file itemroles.h
/// \brief Declares the custom item data roles shared by the models and their views.
///

#pragma once

#include <QVariant>

#include "data/statuskind.h"

namespace ItemRoles {

///
/// \brief Custom roles; views turn IconName and Status into decorations.
///
enum Role {
    /// \brief Icon resource name (QString).
    IconName = Qt::UserRole + 1,
    /// \brief Semantic indicator state (int holding a StatusKind).
    Status,
    /// \brief Identifier of the item (QString).
    Id,
    /// \brief Tree node kind (int holding a TreeNode::Kind).
    NodeKind,
    /// \brief Whether a tree node starts expanded (bool).
    Expanded,
    /// \brief Whether a tree node starts selected (bool).
    Current
};

///
/// \brief Packs a status for the Status role.
/// \param kind Status to pack.
/// \return Variant holding the status as int.
///
inline QVariant statusValue(StatusKind kind)
{
    return static_cast<int>(kind);
}

///
/// \brief Unpacks the Status role of an index.
/// \param value Value read with the Status role.
/// \return Stored status, or StatusKind::None when the role is empty.
///
inline StatusKind statusFrom(const QVariant &value)
{
    return value.isValid() ? static_cast<StatusKind>(value.toInt()) : StatusKind::None;
}

}
