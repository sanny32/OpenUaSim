// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectnodes.h
/// \brief Declares helpers over the node tree of a project.
///

#pragma once

#include <QList>
#include <QSet>
#include <QStringList>

#include "data/projectconfig.h"

namespace ProjectNodes {

///
/// \brief Gives every node without a node id one derived from its browse path.
/// \param nodes Node tree to complete in place.
/// \return Number of node ids assigned; ids such as "ns=1;s=Line.Motor.Speed" get a numeric
///         suffix when the path is already taken.
///
int assignMissingNodeIds(QList<NodeDefinition> &nodes);

///
/// \brief Counts the nodes of a tree.
/// \param nodes Node tree.
/// \return Number of nodes at every depth.
///
int count(const QList<NodeDefinition> &nodes);

///
/// \brief Finds a node by id.
/// \param nodes Node tree.
/// \param nodeId Node id to look for.
/// \return Node, or null when no node has that id.
///
const NodeDefinition *find(const QList<NodeDefinition> &nodes, const QString &nodeId);

///
/// \brief Returns the node ids used in a tree.
/// \param nodes Node tree.
/// \return Non-empty node ids.
///
QSet<QString> nodeIds(const QList<NodeDefinition> &nodes);

///
/// \brief Returns the browse names of the children of a container.
/// \param nodes Node tree.
/// \param parentId Folder or object id; empty for the top level.
/// \return Browse names, or empty when the parent is unknown.
///
QStringList childNames(const QList<NodeDefinition> &nodes, const QString &parentId);

///
/// \brief Adds a node below a container.
/// \param nodes Node tree to extend.
/// \param parentId Folder or object id; empty for the top level.
/// \param node Node to add; an empty node id is assigned from the browse path.
/// \param error Receives the reason on failure; may be null.
/// \return Node id of the added node, or empty when the parent is unknown, a variable or a method,
///         the browse name is empty or used by a sibling, the node id is already used,
///         the data type is unsupported, the array length is outside 0..kMaxArrayLength, the alarm
///         settings are refused by AlarmRules::problem(), or a
///         method would go to the top level.
///
QString insert(QList<NodeDefinition> &nodes, const QString &parentId, const NodeDefinition &node,
               QString *error = nullptr);

///
/// \brief Removes a node with its children.
/// \param nodes Node tree to shrink.
/// \param nodeId Node to remove.
/// \return False when no node has that id.
///
bool remove(QList<NodeDefinition> &nodes, const QString &nodeId);

///
/// \brief Returns the folder or object holding a node.
/// \param nodes Node tree.
/// \param nodeId Node to look up.
/// \return Parent node id; empty for top-level and unknown nodes.
///
QString parentId(const QList<NodeDefinition> &nodes, const QString &nodeId);

///
/// \brief Copies a node with its children for inserting it elsewhere.
/// \param node Node to copy.
/// \param takenNames Browse names already used where the copy goes.
/// \return Copy without node ids, renamed to "<name>_copy", "<name>_copy2", ... when its name is taken.
///
NodeDefinition detachedCopy(const NodeDefinition &node, const QStringList &takenNames);

///
/// \brief Replaces the editable fields of a variable.
/// \param nodes Node tree holding the variable.
/// \param variable Variable with its node id and the new browse name, icon, data type, access,
///        array length, initial value, quality, historizing, simulation and alarm; the initial value is converted
///        to the data type.
/// \param error Receives the reason on failure; may be null.
/// \return False when the node is unknown or not a variable, the browse name is empty or used
///         by a sibling, the data type is unsupported, the array length is outside 0..kMaxArrayLength, or
///         the alarm settings are refused by AlarmRules::problem().
///
bool updateVariable(QList<NodeDefinition> &nodes, const NodeDefinition &variable, QString *error = nullptr);

///
/// \brief Gives a folder, object or variable a new browse name; its node id is kept.
/// \param nodes Node tree holding the node.
/// \param nodeId Node to rename.
/// \param browseName New name.
/// \param error Receives the reason on failure; may be null.
/// \return False when the node is unknown or the name is empty or used by a sibling.
///
bool rename(QList<NodeDefinition> &nodes, const QString &nodeId, const QString &browseName, QString *error = nullptr);

}
