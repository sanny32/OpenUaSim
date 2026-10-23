// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file declarednodes.cpp
/// \brief Implements the nodes that the types of project instances declare, and the settings the project keeps for
///        them.
///

#include <QUaServer>

#include "declarednodes.h"
#include "importednodes.h"
#include "project/projectnodes.h"

namespace {

constexpr quint16 kApplicationNamespace = 1;

///
/// \brief Collects the declared variable settings of a tree.
/// \param nodes Node tree.
/// \param result Receives the declared variables.
///
void collectSettings(const QList<NodeDefinition> &nodes, QList<NodeDefinition> &result)
{
    for (const NodeDefinition &node : nodes) {
        if (node.declared && node.kind == NodeDefinition::Kind::Variable)
            result.append(node);
        collectSettings(node.children, result);
    }
}

///
/// \brief Finds the tree node of a project node among siblings.
/// \param tree Sibling tree nodes.
/// \param nodeId Node id to look for.
/// \return Tree node, or null.
///
TreeNode *findTreeNode(QList<TreeNode> &tree, const QString &nodeId)
{
    for (TreeNode &node : tree) {
        if (node.id == nodeId)
            return &node;
    }
    return nullptr;
}

///
/// \brief Adds the declared nodes of the instances among project siblings and below them.
/// \param server Server holding the instances.
/// \param settings Declared variable settings of the project.
/// \param nodes Project siblings.
/// \param addressSpace Built nodes.
/// \param tree Tree nodes of the siblings.
/// \param variables Receives the declared variables keyed by the node id of their owner.
///
void collectBelow(QUaServer &server, const QList<NodeDefinition> &settings, const QList<NodeDefinition> &nodes,
                  const AddressSpace &addressSpace, QList<TreeNode> &tree,
                  QHash<QString, QList<VariableInfo>> &variables)
{
    for (const NodeDefinition &node : nodes) {
        if (node.declared || node.kind == NodeDefinition::Kind::Variable || node.kind == NodeDefinition::Kind::Method)
            continue;
        TreeNode *treeNode = findTreeNode(tree, node.nodeId);
        if (!treeNode)
            continue;
        collectBelow(server, settings, node.children, addressSpace, treeNode->children, variables);
        if (node.typeDefinition.isEmpty())
            continue;
        if (const auto *instance = server.nodeById<QUaBaseObject>(QUaNodeId(node.nodeId)))
            ImportedNodes::collectDeclared(server, settings, addressSpace, *instance, *treeNode, variables);
    }
}

}

namespace DeclaredNodes {

///
/// \brief Lists the settings the project keeps for variables that the types of its instances declare.
/// \param nodes Project node tree.
/// \return Declared variable definitions of every depth, with their server node ids.
///
QList<NodeDefinition> settings(const QList<NodeDefinition> &nodes)
{
    QList<NodeDefinition> result;
    collectSettings(nodes, result);
    return result;
}

///
/// \brief Adds the nodes that the types of built project instances declare to the tree and the variable lists.
/// \param server Server holding the instances.
/// \param nodes Project node tree.
/// \param addressSpace Built nodes providing the live state of variables with settings.
/// \param tree Tree nodes of the project, as SnapshotBuilder::addressSpaceTree() builds them below Objects.
/// \param variables Variables keyed by the node id of their owner, receiving the declared ones.
///
void collect(QUaServer &server, const QList<NodeDefinition> &nodes, const AddressSpace &addressSpace,
             QList<TreeNode> &tree, QHash<QString, QList<VariableInfo>> &variables)
{
    collectBelow(server, settings(nodes), nodes, addressSpace, tree, variables);
}

///
/// \brief Keeps the first settings of a variable that the type of a project instance declares.
/// \param nodes Project node tree receiving the settings below the instance, with settings entries for the declared
///        folders and objects between.
/// \param variable Declared server variable.
/// \param settings Settings with the node id, browse name and data type of the variable.
/// \return False when no project instance declares the variable.
///
bool keep(QList<NodeDefinition> &nodes, const QUaBaseDataVariable &variable, const NodeDefinition &settings)
{
    if (variable.browseName().namespaceIndex() == kApplicationNamespace)
        return false;
    QList<const QUaNode *> containers;
    const auto *owner = qobject_cast<const QUaNode *>(variable.parent());
    while (owner && !ProjectNodes::find(nodes, owner->nodeId().toXmlString())) {
        if (owner->browseName().namespaceIndex() == kApplicationNamespace)
            return false;
        containers.prepend(owner);
        owner = qobject_cast<const QUaNode *>(owner->parent());
    }
    if (!owner)
        return false;

    QString parentId = owner->nodeId().toXmlString();
    for (const QUaNode *container : std::as_const(containers)) {
        NodeDefinition entry;
        entry.kind = qobject_cast<const QUaFolderObject *>(container) ? NodeDefinition::Kind::Folder
                                                                      : NodeDefinition::Kind::Object;
        entry.browseName = container->browseName().name();
        entry.nodeId = container->nodeId().toXmlString();
        entry.declared = true;
        if (ProjectNodes::insert(nodes, parentId, entry).isEmpty())
            return false;
        parentId = entry.nodeId;
    }
    NodeDefinition entry = settings;
    entry.declared = true;
    return !ProjectNodes::insert(nodes, parentId, entry).isEmpty();
}

}
