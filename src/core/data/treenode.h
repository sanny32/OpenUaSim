// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file treenode.h
/// \brief Declares the plain tree description consumed by TreeModel.
///

#pragma once

#include <QList>
#include <QString>

///
/// \brief One node of a navigation tree, together with its children.
///
struct TreeNode
{
    ///
    /// \brief Role of the node, used by pages to decide what a selection shows.
    ///
    enum class Kind { Folder, Object, Variable, Type, Category, Method, Property };

    /// \brief Text shown in the first column.
    QString text;
    /// \brief Optional text shown right-aligned in the second column.
    QString trailing;
    /// \brief Resource name of the icon, without directory and extension.
    QString icon;
    /// \brief Identifier the owning page resolves the node by.
    QString id;
    /// \brief Role of the node.
    Kind kind = Kind::Folder;
    /// \brief Whether the view should show the node expanded initially.
    bool expanded = false;
    /// \brief Whether the view should start with this node selected.
    bool current = false;
    /// \brief Child nodes.
    QList<TreeNode> children;
};
