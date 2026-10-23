// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file importednodes.cpp
/// \brief Implements the description of nodes that imported NodeSet2 models add under the Objects folder.
///

#include <QUaServer>

#include "addressspace.h"
#include "importednodes.h"
#include "modelvariables.h"
#include "snapshotbuilder.h"
#include "variableproperties.h"

namespace {

constexpr quint16 kApplicationNamespace = 1;
constexpr quint16 kFirstImportedNamespace = 2;

///
/// \brief Returns the built-in data types the simulation writes.
/// \return Names keyed by their numeric node id in namespace 0 (OPC UA Part 6, 5.1.2).
///
const QHash<QString, QString> &builtInDataTypes()
{
    static const QHash<QString, QString> names {
        { QStringLiteral("i=1"), QStringLiteral("Boolean") },  { QStringLiteral("i=2"), QStringLiteral("SByte") },
        { QStringLiteral("i=3"), QStringLiteral("Byte") },     { QStringLiteral("i=4"), QStringLiteral("Int16") },
        { QStringLiteral("i=5"), QStringLiteral("UInt16") },   { QStringLiteral("i=6"), QStringLiteral("Int32") },
        { QStringLiteral("i=7"), QStringLiteral("UInt32") },   { QStringLiteral("i=8"), QStringLiteral("Int64") },
        { QStringLiteral("i=9"), QStringLiteral("UInt64") },   { QStringLiteral("i=10"), QStringLiteral("Float") },
        { QStringLiteral("i=11"), QStringLiteral("Double") },  { QStringLiteral("i=12"), QStringLiteral("String") },
        { QStringLiteral("i=13"), QStringLiteral("DateTime") },
    };
    return names;
}

///
/// \brief Describes the nodes of imported models, with the settings the project keeps for their variables.
///
class ModelNodeCollector
{
public:
    ModelNodeCollector(const QUaServer &server, const QList<NodeDefinition> &settings,
                       const AddressSpace &addressSpace);
    TreeNode containerNode(const QUaBaseObject &node, QHash<QString, QList<VariableInfo>> &variables) const;
    void addChildren(const QUaBaseObject &node, bool declaredOnly, TreeNode &result,
                     QHash<QString, QList<VariableInfo>> &variables) const;

private:
    const QUaServer &_server;
    QHash<QString, const NodeDefinition *> _settings;
    const AddressSpace &_addressSpace;
};

///
/// \brief Prepares the description of model nodes.
/// \param server Server holding the models.
/// \param settings Model variable settings with server node ids, as ModelVariables::resolve() returns them.
/// \param addressSpace Built nodes providing the live state of variables with settings.
///
ModelNodeCollector::ModelNodeCollector(const QUaServer &server, const QList<NodeDefinition> &settings,
                                       const AddressSpace &addressSpace)
    : _server(server)
    , _addressSpace(addressSpace)
{
    for (const NodeDefinition &variable : settings)
        _settings.insert(variable.nodeId, &variable);
}

///
/// \brief Describes an imported folder or object with its children and records its variables.
/// \param node Server folder or object.
/// \param variables Receives the variables keyed by the node id of their owner.
/// \return Tree node.
///
TreeNode ModelNodeCollector::containerNode(const QUaBaseObject &node,
                                           QHash<QString, QList<VariableInfo>> &variables) const
{
    TreeNode result;
    result.text = node.browseName().name();
    result.icon = QStringLiteral("folder");
    result.id = node.nodeId();
    result.kind = qobject_cast<const QUaFolderObject *>(&node) ? TreeNode::Kind::Folder : TreeNode::Kind::Object;
    addChildren(node, false, result, variables);
    return result;
}

///
/// \brief Describes the children of a folder or object and records its variables after those listed already.
/// \param node Server folder or object.
/// \param declaredOnly Whether only children outside the application namespace, which a type declares, are added.
/// \param result Tree node of the folder or object receiving the children.
/// \param variables Receives the variables keyed by the node id of their owner.
///
void ModelNodeCollector::addChildren(const QUaBaseObject &node, bool declaredOnly, TreeNode &result,
                                     QHash<QString, QList<VariableInfo>> &variables) const
{
    QList<VariableInfo> owned;
    const QList<QUaNode *> children = node.browseChildren();
    for (const QUaNode *child : children) {
        if (declaredOnly && child->browseName().namespaceIndex() == kApplicationNamespace)
            continue;
        if (const auto *variable = qobject_cast<const QUaBaseDataVariable *>(child)) {
            owned.append(ImportedNodes::variableInfo(_server, *variable, _settings.value(variable->nodeId()),
                                                     _addressSpace));
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
        variables[result.id] += owned;
}

}

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
             QList<TreeNode> &tree, QHash<QString, QList<VariableInfo>> &variables)
{
    const ModelNodeCollector collector(server, settings, addressSpace);
    const QList<QUaNode *> roots = server.objectsFolder()->browseChildren();
    for (const QUaNode *root : roots) {
        if (root->nodeId().namespaceIndex() < kFirstImportedNamespace)
            continue;
        if (const auto *object = qobject_cast<const QUaBaseObject *>(root))
            tree.append(collector.containerNode(*object, variables));
    }
}

///
/// \brief Describes the folders, objects and variables that the type of a project instance declares.
/// \param server Server holding the instance.
/// \param settings Settings the project keeps for declared variables, with their server node ids.
/// \param addressSpace Built nodes providing the live state of variables with settings.
/// \param instance Built project folder or object of a type.
/// \param node Tree node of the instance; receives the declared children after its own.
/// \param variables Receives the declared variables keyed by the node id of their owner, after the listed ones.
///
void collectDeclared(const QUaServer &server, const QList<NodeDefinition> &settings, const AddressSpace &addressSpace,
                     const QUaBaseObject &instance, TreeNode &node, QHash<QString, QList<VariableInfo>> &variables)
{
    ModelNodeCollector(server, settings, addressSpace).addChildren(instance, true, node, variables);
}

///
/// \brief Describes a variable of an imported model.
/// \param server Server holding the model.
/// \param variable Model variable.
/// \param settings Settings the project keeps for the variable, with its server node id; null for none.
/// \param addressSpace Built nodes providing the live state of a variable with settings.
/// \return Description with the live value; read-only when the simulation cannot drive the variable.
///
VariableInfo variableInfo(const QUaServer &server, const QUaBaseDataVariable &variable,
                          const NodeDefinition *settings, const AddressSpace &addressSpace)
{
    VariableInfo info;
    if (settings) {
        info = SnapshotBuilder::variableInfo(*settings, addressSpace);
    } else {
        const NodeDefinition model = ModelVariables::describe(server, variable).variable;
        info.nodeId = variable.nodeId();
        info.displayName = model.browseName;
        info.icon = model.icon;
        info.value = variable.value();
        info.dataType = model.dataType;
        info.arrayLength = model.arrayLength;
        info.access = model.access;
        info.quality = AddressSpace::qualityOf(variable);
        info.configuredQuality = info.quality;
        info.historizing = model.historizing;
    }
    info.dataTypeName = ModelVariables::dataTypeName(server, variable);
    info.typeName = variable.typeDefinitionBrowseName().name();
    info.properties = VariableProperties::describe(server, variable);
    info.fromModel = true;
    info.readOnly = !ModelVariables::isEditable(server, variable);
    return info;
}

///
/// \brief Returns the name of a built-in data type.
/// \param dataTypeNodeId Node id of the data type, e.g. "i=11" or "ns=0;i=11".
/// \return Name such as "Double", or the node id for other types.
///
QString dataTypeName(const QString &dataTypeNodeId)
{
    QString id = dataTypeNodeId;
    if (id.startsWith(QLatin1String("ns=0;")))
        id.remove(0, 5);
    return builtInDataTypes().value(id, dataTypeNodeId);
}

///
/// \brief Returns the node id of a built-in data type.
/// \param name Name such as "Double".
/// \return Node id such as "i=11", or empty for other names.
///
QString dataTypeNodeId(const QString &name)
{
    return builtInDataTypes().key(name);
}

}
