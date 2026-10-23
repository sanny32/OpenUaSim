// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file variablelisting.cpp
/// \brief Implements the collection of the variables a folder or object lists.
///

#include "variablelisting.h"

namespace {

///
/// \brief Appends the variables of the folders, objects and variables below a node.
/// \param node Node whose children are visited.
/// \param path Path of the node relative to the listed container; empty for the container itself.
/// \param variables Variables keyed by the node id of their folder, object or variable.
/// \param listing Receives the variables and their owner paths.
///
void collectNested(const TreeNode &node, const QString &path,
                   const QHash<QString, QList<VariableInfo>> &variables, VariableListing::Listing &listing)
{
    for (const TreeNode &child : node.children) {
        if (child.kind != TreeNode::Kind::Folder && child.kind != TreeNode::Kind::Object
            && child.kind != TreeNode::Kind::Variable)
            continue;
        const QString childPath = path.isEmpty() ? child.text : path + QLatin1Char('/') + child.text;
        const QList<VariableInfo> owned = variables.value(child.id);
        for (const VariableInfo &variable : owned)
            listing.locations.insert(variable.nodeId, childPath);
        listing.variables.append(owned);
        collectNested(child, childPath, variables, listing);
    }
}

}

namespace VariableListing {

///
/// \brief Finds a node of a tree.
/// \param nodes Top-level nodes.
/// \param id Node id.
/// \return The first node with the id in depth-first order, or nullptr.
///
const TreeNode *find(const QList<TreeNode> &nodes, const QString &id)
{
    for (const TreeNode &node : nodes) {
        if (node.id == id)
            return &node;
        if (const TreeNode *found = find(node.children, id))
            return found;
    }
    return nullptr;
}

///
/// \brief Collects the variables a folder or object lists.
/// \param container Folder, object or variable node.
/// \param variables Variables keyed by the node id of their folder, object or variable.
/// \param nested Whether to include the variables of the folders, objects and variables below the container.
/// \return Listed variables with the owner paths of the nested ones.
///
Listing collect(const TreeNode &container, const QHash<QString, QList<VariableInfo>> &variables, bool nested)
{
    Listing listing;
    listing.variables = variables.value(container.id);
    if (nested)
        collectNested(container, QString(), variables, listing);
    return listing;
}

}
