// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file declarednodes.h
/// \brief Declares the nodes that the types of project instances declare, and the settings the project keeps for
///        them.
///

#pragma once

#include <QHash>
#include <QList>
#include <QString>

#include "data/projectconfig.h"
#include "data/treenode.h"
#include "data/variableinfo.h"

class AddressSpace;
class QUaBaseDataVariable;
class QUaServer;

namespace DeclaredNodes {

///
/// \brief Lists the settings the project keeps for variables that the types of its instances declare.
/// \param nodes Project node tree.
/// \return Declared variable definitions of every depth, with their server node ids.
///
QList<NodeDefinition> settings(const QList<NodeDefinition> &nodes);

///
/// \brief Adds the nodes that the types of built project instances declare to the tree and the variable lists.
/// \param server Server holding the instances.
/// \param nodes Project node tree.
/// \param addressSpace Built nodes providing the live state of variables with settings.
/// \param tree Tree nodes of the project, as SnapshotBuilder::addressSpaceTree() builds them below Objects.
/// \param variables Variables keyed by the node id of their owner, receiving the declared ones.
///
void collect(QUaServer &server, const QList<NodeDefinition> &nodes, const AddressSpace &addressSpace,
             QList<TreeNode> &tree, QHash<QString, QList<VariableInfo>> &variables);

///
/// \brief Keeps the first settings of a variable that the type of a project instance declares.
/// \param nodes Project node tree receiving the settings below the instance, with settings entries for the declared
///        folders and objects between.
/// \param variable Declared server variable.
/// \param settings Settings with the node id, browse name and data type of the variable.
/// \return False when no project instance declares the variable.
///
bool keep(QList<NodeDefinition> &nodes, const QUaBaseDataVariable &variable, const NodeDefinition &settings);

}
