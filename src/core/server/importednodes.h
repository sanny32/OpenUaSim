// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file importednodes.h
/// \brief Declares the description of nodes that imported NodeSet2 models add under the Objects folder.
///

#pragma once

#include <QHash>
#include <QList>

#include "data/projectconfig.h"
#include "data/treenode.h"
#include "data/variableinfo.h"

class AddressSpace;
class QUaBaseDataVariable;
class QUaServer;

namespace ImportedNodes {

///
/// \brief Describes the folders, objects and variables of imported models under the Objects folder.
/// \param server Server holding the models; nodes of namespaces 0 and 1 are skipped.
/// \param settings Model variable settings with server node ids, as ModelVariables::resolve() returns them.
/// \param addressSpace Built nodes providing the live state of variables with settings.
/// \param tree Receives the top-level imported nodes with their children.
/// \param variables Receives the variables keyed by the node id of their folder or object.
///
void collect(const QUaServer &server, const QList<NodeDefinition> &settings, const AddressSpace &addressSpace,
             QList<TreeNode> &tree, QHash<QString, QList<VariableInfo>> &variables);

///
/// \brief Describes a variable of an imported model.
/// \param server Server holding the model.
/// \param variable Model variable.
/// \param settings Settings the project keeps for the variable, with its server node id; null for none.
/// \param addressSpace Built nodes providing the live state of a variable with settings.
/// \return Description with the live value; read-only when the simulation cannot drive the variable.
///
VariableInfo variableInfo(const QUaServer &server, const QUaBaseDataVariable &variable,
                          const NodeDefinition *settings, const AddressSpace &addressSpace);

///
/// \brief Returns the name of a built-in data type.
/// \param dataTypeNodeId Node id of the data type, e.g. "i=11" or "ns=0;i=11".
/// \return Name such as "Double", or the node id for other types.
///
QString dataTypeName(const QString &dataTypeNodeId);

}
