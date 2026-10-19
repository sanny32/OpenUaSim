// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file importednodes.h
/// \brief Declares the description of nodes that imported NodeSet2 models add under the Objects folder.
///

#pragma once

#include <QHash>
#include <QList>

#include "data/treenode.h"
#include "data/variableinfo.h"

class QUaServer;

namespace ImportedNodes {

///
/// \brief Describes the folders, objects and variables of imported models under the Objects folder.
/// \param server Server holding the models; nodes of namespaces 0 and 1 are skipped.
/// \param tree Receives the top-level imported nodes with their children.
/// \param variables Receives the read-only variables keyed by the node id of their folder or object.
///
void collect(const QUaServer &server, QList<TreeNode> &tree, QHash<QString, QList<VariableInfo>> &variables);

///
/// \brief Returns the name of a built-in data type.
/// \param dataTypeNodeId Node id of the data type, e.g. "i=11" or "ns=0;i=11".
/// \return Name such as "Double", or the node id for other types.
///
QString dataTypeName(const QString &dataTypeNodeId);

}
