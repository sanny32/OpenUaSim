// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file importednodes.cpp
/// \brief Implements the description of nodes that imported NodeSet2 models add under the Objects folder.
///

#include <QUaServer>

#include "addressspace.h"
#include "importednodes.h"
#include "simulation/valuecast.h"

namespace {

constexpr quint16 kFirstImportedNamespace = 2;

///
/// \brief Returns the access level text of a variable.
/// \param variable Variable to describe.
/// \return "Read", "Write" or "Read/Write".
///
QString accessText(const QUaBaseVariable &variable)
{
    if (variable.readAccess() && variable.writeAccess())
        return QStringLiteral("Read/Write");
    return variable.writeAccess() ? QStringLiteral("Write") : QStringLiteral("Read");
}

///
/// \brief Describes an imported variable.
/// \param variable Server variable.
/// \return Read-only description with the live value.
///
VariableInfo variableInfo(const QUaBaseDataVariable &variable)
{
    VariableInfo info;
    info.nodeId = variable.nodeId();
    info.displayName = variable.browseName().name();
    info.icon = QStringLiteral("signal");
    info.value = variable.value();
    info.dataType = ImportedNodes::dataTypeName(variable.dataTypeNodeId());
    if (ValueCast::isArray(info.value))
        info.arrayLength = int(info.value.toList().size());
    info.access = accessText(variable);
    info.quality = AddressSpace::qualityOf(variable);
    info.configuredQuality = info.quality;
    info.readOnly = true;
    return info;
}

///
/// \brief Describes an imported folder or object with its children and records its variables.
/// \param node Server folder or object.
/// \param variables Receives the variables keyed by the node id of their owner.
/// \return Tree node.
///
TreeNode containerNode(const QUaBaseObject &node, QHash<QString, QList<VariableInfo>> &variables)
{
    TreeNode result;
    result.text = node.browseName().name();
    result.icon = QStringLiteral("folder");
    result.id = node.nodeId();
    result.kind = qobject_cast<const QUaFolderObject *>(&node) ? TreeNode::Kind::Folder : TreeNode::Kind::Object;

    QList<VariableInfo> owned;
    const QList<QUaNode *> children = node.browseChildren();
    for (const QUaNode *child : children) {
        if (const auto *variable = qobject_cast<const QUaBaseDataVariable *>(child)) {
            owned.append(variableInfo(*variable));
            TreeNode leaf;
            leaf.text = owned.constLast().displayName;
            leaf.icon = owned.constLast().icon;
            leaf.id = owned.constLast().nodeId;
            leaf.kind = TreeNode::Kind::Variable;
            result.children.append(leaf);
        } else if (const auto *object = qobject_cast<const QUaBaseObject *>(child)) {
            result.children.append(containerNode(*object, variables));
        }
    }
    if (!owned.isEmpty())
        variables.insert(result.id, owned);
    return result;
}

}

namespace ImportedNodes {

///
/// \brief Describes the folders, objects and variables of imported models under the Objects folder.
/// \param server Server holding the models; nodes of namespaces 0 and 1 are skipped.
/// \param tree Receives the top-level imported nodes with their children.
/// \param variables Receives the read-only variables keyed by the node id of their folder or object.
///
void collect(const QUaServer &server, QList<TreeNode> &tree, QHash<QString, QList<VariableInfo>> &variables)
{
    const QList<QUaNode *> roots = server.objectsFolder()->browseChildren();
    for (const QUaNode *root : roots) {
        if (root->nodeId().namespaceIndex() < kFirstImportedNamespace)
            continue;
        if (const auto *object = qobject_cast<const QUaBaseObject *>(root))
            tree.append(containerNode(*object, variables));
    }
}

///
/// \brief Returns the name of a built-in data type.
/// \param dataTypeNodeId Node id of the data type, e.g. "i=11" or "ns=0;i=11".
/// \return Name such as "Double", or the node id for other types.
///
QString dataTypeName(const QString &dataTypeNodeId)
{
    // Numeric ids of the built-in types in namespace 0 (OPC UA Part 6, 5.1.2).
    static const QHash<QString, QString> names {
        { QStringLiteral("i=1"), QStringLiteral("Boolean") },  { QStringLiteral("i=2"), QStringLiteral("SByte") },
        { QStringLiteral("i=3"), QStringLiteral("Byte") },     { QStringLiteral("i=4"), QStringLiteral("Int16") },
        { QStringLiteral("i=5"), QStringLiteral("UInt16") },   { QStringLiteral("i=6"), QStringLiteral("Int32") },
        { QStringLiteral("i=7"), QStringLiteral("UInt32") },   { QStringLiteral("i=8"), QStringLiteral("Int64") },
        { QStringLiteral("i=9"), QStringLiteral("UInt64") },   { QStringLiteral("i=10"), QStringLiteral("Float") },
        { QStringLiteral("i=11"), QStringLiteral("Double") },  { QStringLiteral("i=12"), QStringLiteral("String") },
        { QStringLiteral("i=13"), QStringLiteral("DateTime") },
    };
    QString id = dataTypeNodeId;
    if (id.startsWith(QLatin1String("ns=0;")))
        id.remove(0, 5);
    return names.value(id, dataTypeNodeId);
}

}
