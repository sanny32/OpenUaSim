// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectnodes.cpp
/// \brief Implements helpers over the node tree of a project.
///

#include <QSet>

#include "projectnodes.h"

namespace {

///
/// \brief Collects the node ids already present in a tree.
/// \param nodes Node tree.
/// \param ids Receives the non-empty ids.
///
void collectIds(const QList<NodeDefinition> &nodes, QSet<QString> &ids)
{
    for (const NodeDefinition &node : nodes) {
        if (!node.nodeId.isEmpty())
            ids.insert(node.nodeId);
        collectIds(node.children, ids);
    }
}

///
/// \brief Assigns ids below one parent.
/// \param nodes Siblings to complete.
/// \param parentPath Dotted browse path of their parent; empty at the root.
/// \param used Ids already taken; extended with the assigned ones.
/// \return Number of ids assigned.
///
int assign(QList<NodeDefinition> &nodes, const QString &parentPath, QSet<QString> &used)
{
    int assigned = 0;
    for (NodeDefinition &node : nodes) {
        const QString path = parentPath.isEmpty() ? node.browseName
                                                  : parentPath + QLatin1Char('.') + node.browseName;
        if (node.nodeId.isEmpty()) {
            QString candidate = QStringLiteral("ns=1;s=%1").arg(path);
            for (int suffix = 2; used.contains(candidate); ++suffix)
                candidate = QStringLiteral("ns=1;s=%1_%2").arg(path).arg(suffix);
            node.nodeId = candidate;
            used.insert(candidate);
            ++assigned;
        }
        assigned += assign(node.children, path, used);
    }
    return assigned;
}

}

namespace ProjectNodes {

///
/// \brief Gives every node without a node id one derived from its browse path.
/// \param nodes Node tree to complete in place.
/// \return Number of node ids assigned; ids such as "ns=1;s=Line.Motor.Speed" get a numeric
///         suffix when the path is already taken.
///
int assignMissingNodeIds(QList<NodeDefinition> &nodes)
{
    QSet<QString> used;
    collectIds(nodes, used);
    return assign(nodes, QString(), used);
}

///
/// \brief Counts the nodes of a tree.
/// \param nodes Node tree.
/// \return Number of nodes at every depth.
///
int count(const QList<NodeDefinition> &nodes)
{
    int total = 0;
    for (const NodeDefinition &node : nodes)
        total += 1 + count(node.children);
    return total;
}

}
