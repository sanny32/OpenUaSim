// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file importednodes.cpp
/// \brief Implements the description of nodes that imported NodeSet2 models add under the Objects folder.
///

#include <QUaServer>
#include <QUaProperty>

#include <QSet>

#include <algorithm>

#include "addressspace.h"
#include "importednodes.h"
#include "modelvariables.h"
#include "snapshotbuilder.h"
#include "typecatalog.h"
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
/// \brief Remembers nodes already represented by project and declared instance branches.
///
void collectTreeIds(const QList<TreeNode> &nodes, QSet<QString> &ids)
{
    for (const TreeNode &node : nodes) {
        ids.insert(node.id);
        collectTreeIds(node.children, ids);
    }
}

///
/// \brief Adds imported descendants without replacing project metadata or duplicating existing branches.
///
void mergeBranches(QList<TreeNode> &tree, const QList<TreeNode> &imported)
{
    for (const TreeNode &node : imported) {
        auto existing = std::find_if(tree.begin(), tree.end(),
                                     [&node](const TreeNode &entry) { return entry.id == node.id; });
        if (existing == tree.end())
            tree.append(node);
        else
            mergeBranches(existing->children, node.children);
    }
}

///
/// \brief Describes the nodes of imported models, with the settings the project keeps for their variables.
///
class ModelNodeCollector
{
public:
    ModelNodeCollector(QUaServer &server, const QList<NodeDefinition> &settings,
                       const AddressSpace &addressSpace);
    TreeNode containerNode(const QUaBaseObject &node, QHash<QString, QList<VariableInfo>> &variables) const;
    void addChildren(const QUaBaseObject &node, bool declaredOnly, TreeNode &result,
                     QHash<QString, QList<VariableInfo>> &variables) const;
    void collectImported(QList<TreeNode> &tree, QHash<QString, QList<VariableInfo>> &variables);

private:
    TreeNode importedBranch(const QUaNodeId &nodeId, const QString &parentId, bool includeStandard,
                            QHash<QString, QList<VariableInfo>> &variables);
    bool isChildReference(const QUaNodeReference &reference);
    VariableInfo describeVariable(const QUaBaseVariable &variable, const QString &parentId) const;

    QUaServer &_server;
    QHash<QString, const NodeDefinition *> _settings;
    const AddressSpace &_addressSpace;
    QSet<QUaNodeId> _ancestors;
    QSet<QString> _existingNodes;
    QHash<QUaNodeId, bool> _childReferenceTypes;
};

///
/// \brief Prepares the description of model nodes.
/// \param server Server holding the models.
/// \param settings Model variable settings with server node ids, as ModelVariables::resolve() returns them.
/// \param addressSpace Built nodes providing the live state of variables with settings.
///
ModelNodeCollector::ModelNodeCollector(QUaServer &server, const QList<NodeDefinition> &settings,
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

///
/// \brief Retains standard ancestor paths only when they lead to imported instance nodes.
///
void ModelNodeCollector::collectImported(QList<TreeNode> &tree, QHash<QString, QList<VariableInfo>> &variables)
{
    collectTreeIds(tree, _existingNodes);
    const TreeNode objects = importedBranch(QUaNodeId(0, quint32(UA_NS0ID_OBJECTSFOLDER)), QString(), false, variables);
    mergeBranches(tree, objects.children);
}

///
/// \brief Traverses actual OPC UA references, including ancestors without Qt instances, and breaks reference cycles.
///
TreeNode ModelNodeCollector::importedBranch(const QUaNodeId &nodeId, const QString &parentId, bool includeStandard,
                                           QHash<QString, QList<VariableInfo>> &variables)
{
    if (_ancestors.contains(nodeId))
        return {};
    const QUaNode *node = _server.nodeById(nodeId);
    if (node && VariableProperties::isIcon(*node))
        return {};
    const QUaQualifiedName name = _server.browseName(nodeId);
    const bool included = includeStandard || nodeId.namespaceIndex() >= kFirstImportedNamespace
        || name.namespaceIndex() >= kFirstImportedNamespace;
    TreeNode result;
    result.id = nodeId == QUaNodeId(0, quint32(UA_NS0ID_OBJECTSFOLDER)) ? QStringLiteral("i=85") : nodeId.toXmlString();
    result.text = name.name();
    result.icon = QStringLiteral("folder");
    result.kind = TreeNode::Kind::Object;
    _ancestors.insert(nodeId);
    QSet<QUaNodeId> children;
    QUaNodeId typeId;
    const QList<QUaNodeReference> references = _server.references(nodeId);
    for (const QUaNodeReference &reference : references) {
        if (reference.isForward && reference.referenceTypeId == QUaNodeId(0, quint32(UA_NS0ID_HASTYPEDEFINITION)))
            typeId = reference.targetNodeId;
        if (!isChildReference(reference) || children.contains(reference.targetNodeId))
            continue;
        children.insert(reference.targetNodeId);
        TreeNode child = importedBranch(reference.targetNodeId, result.id, included, variables);
        if (!child.id.isEmpty())
            result.children.append(child);
    }
    _ancestors.remove(nodeId);
    if (!included && result.children.isEmpty())
        return {};
    if (const auto *variable = qobject_cast<const QUaBaseVariable *>(node)) {
        const VariableInfo info = describeVariable(*variable, parentId);
        result.icon = info.icon;
        result.kind = info.property ? TreeNode::Kind::Property : TreeNode::Kind::Variable;
        if (!_existingNodes.contains(result.id)) {
            QList<VariableInfo> &owned = variables[parentId];
            if (std::none_of(owned.cbegin(), owned.cend(),
                             [&info](const VariableInfo &entry) { return entry.nodeId == info.nodeId; }))
                owned.append(info);
        }
    } else if (_server.isSubtypeOf(typeId, QUaNodeId(0, quint32(UA_NS0ID_FOLDERTYPE)))) {
        result.kind = TreeNode::Kind::Folder;
    } else if (!node && typeId.isNull()) {
        result.kind = TreeNode::Kind::Method;
        result.icon = QStringLiteral("play");
    }
    return result;
}

///
/// \brief Restricts traversal to instance hierarchy references, excluding type inheritance.
///
bool ModelNodeCollector::isChildReference(const QUaNodeReference &reference)
{
    if (!reference.isForward)
        return false;
    auto found = _childReferenceTypes.constFind(reference.referenceTypeId);
    if (found == _childReferenceTypes.cend()) {
        const bool hierarchical = _server.isSubtypeOf(reference.referenceTypeId,
                                                      QUaNodeId(0, quint32(UA_NS0ID_HIERARCHICALREFERENCES)))
            && !_server.isSubtypeOf(reference.referenceTypeId, QUaNodeId(0, quint32(UA_NS0ID_HASSUBTYPE)));
        found = _childReferenceTypes.insert(reference.referenceTypeId, hierarchical);
    }
    return *found;
}

///
/// \brief Keeps model variable settings and exposes object properties as read-only values.
///
VariableInfo ModelNodeCollector::describeVariable(const QUaBaseVariable &variable, const QString &parentId) const
{
    if (const auto *data = qobject_cast<const QUaBaseDataVariable *>(&variable))
        return ImportedNodes::variableInfo(_server, *data, _settings.value(data->nodeId()), _addressSpace);
    VariableInfo info = VariableProperties::describeChild(_server, variable);
    if (!_server.nodeById<QUaBaseDataVariable>(QUaNodeId(parentId)))
        info.readOnly = true;
    return info;
}

}

namespace ImportedNodes {

///
/// \brief Describes imported instance branches, retaining the standard ancestors needed to reach them.
/// \param server Server holding the models.
/// \param settings Model variable settings with server node ids, as ModelVariables::resolve() returns them.
/// \param addressSpace Built nodes providing the live state of variables with settings.
/// \param tree Receives imported nodes merged with existing project branches.
/// \param variables Receives the variables keyed by the node id of their folder or object.
///
void collect(QUaServer &server, const QList<NodeDefinition> &settings, const AddressSpace &addressSpace,
             QList<TreeNode> &tree, QHash<QString, QList<VariableInfo>> &variables)
{
    ModelNodeCollector(server, settings, addressSpace).collectImported(tree, variables);
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
void collectDeclared(QUaServer &server, const QList<NodeDefinition> &settings, const AddressSpace &addressSpace,
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
    info.typeIcon = TypeCatalog::variableTypeIcon(server, variable.typeDefinitionNodeId().toXmlString());
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
