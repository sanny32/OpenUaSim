// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file variablelisting.h
/// \brief Declares the collection of the variables a folder or object lists.
///

#pragma once

#include <QHash>
#include <QList>
#include <QString>

#include "data/treenode.h"
#include "data/variableinfo.h"

namespace VariableListing {

///
/// \brief Variables listed for one folder or object.
///
struct Listing
{
    /// \brief Variables in tree order: the container's own first, then those of each nested node.
    QList<VariableInfo> variables;
    /// \brief Path of the owner relative to the container, keyed by variable node id; own variables have none.
    QHash<QString, QString> locations;
};

///
/// \brief Finds a node of a tree.
/// \param nodes Top-level nodes.
/// \param id Node id.
/// \return The first node with the id in depth-first order, or nullptr.
///
const TreeNode *find(const QList<TreeNode> &nodes, const QString &id);

///
/// \brief Collects the variables a folder or object lists.
/// \param container Folder, object or variable node.
/// \param variables Variables keyed by the node id of their folder, object or variable.
/// \param nested Whether to include the variables of the folders, objects and variables below the container.
/// \return Listed variables with the owner paths of the nested ones.
///
Listing collect(const TreeNode &container, const QHash<QString, QList<VariableInfo>> &variables, bool nested);

}
