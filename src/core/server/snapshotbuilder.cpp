// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file snapshotbuilder.cpp
/// \brief Implements the assembly of the state the UI shows from the project and the running server.
///

#include <QCoreApplication>

#include <QUaProperty>
#include <QUaServer>

#include "addressspace.h"
#include "declarednodes.h"
#include "importednodes.h"
#include "listenersettings.h"
#include "modelvariables.h"
#include "simserver.h"
#include "snapshotbuilder.h"
#include "typecatalog.h"
#include "variableproperties.h"

namespace {

///
/// \brief Returns the tree role of a node kind.
/// \param kind Node kind.
/// \return Matching tree node kind.
///
TreeNode::Kind treeKind(NodeDefinition::Kind kind)
{
    switch (kind) {
    case NodeDefinition::Kind::Folder:   return TreeNode::Kind::Folder;
    case NodeDefinition::Kind::Object:   return TreeNode::Kind::Object;
    case NodeDefinition::Kind::Variable: return TreeNode::Kind::Variable;
    case NodeDefinition::Kind::Method:   return TreeNode::Kind::Method;
    }
    return TreeNode::Kind::Folder;
}

///
/// \brief Returns the icon of a node, falling back to one for its kind.
/// \param node Node definition.
/// \return Icon resource name.
///
QString iconOf(const NodeDefinition &node)
{
    if (!node.icon.isEmpty())
        return node.icon;
    switch (node.kind) {
    case NodeDefinition::Kind::Variable: return QStringLiteral("signal");
    case NodeDefinition::Kind::Method:   return QStringLiteral("play");
    default:                             return QStringLiteral("folder");
    }
}

///
/// \brief Builds a folder node of namespace 0.
/// \param text Browse name.
/// \param id Node id.
/// \return Collapsed folder.
///
TreeNode standardFolder(const QString &text, const QString &id)
{
    TreeNode node;
    node.text = text;
    node.icon = QStringLiteral("folder");
    node.id = id;
    node.kind = TreeNode::Kind::Folder;
    return node;
}

///
/// \brief Converts a project node and its children into tree nodes.
/// \param node Node definition.
/// \return Tree node.
///
TreeNode treeNode(const NodeDefinition &node)
{
    TreeNode result;
    result.text = node.browseName;
    result.icon = iconOf(node);
    result.id = node.nodeId;
    result.kind = treeKind(node.kind);
    for (const NodeDefinition &child : node.children) {
        if (!child.declared)
            result.children.append(treeNode(child));
    }
    return result;
}

///
/// \brief Expands the path to the first variable and makes it current.
/// \param nodes Siblings to search.
/// \return True when a variable was found below them.
///
bool selectFirstVariable(QList<TreeNode> &nodes)
{
    for (TreeNode &node : nodes) {
        if (node.kind == TreeNode::Kind::Variable) {
            node.current = true;
            return true;
        }
        if (selectFirstVariable(node.children)) {
            node.expanded = true;
            return true;
        }
    }
    return false;
}

///
/// \brief Collects the variables below each container.
/// \param nodes Siblings to visit.
/// \param addressSpace Built nodes.
/// \param result Receives the variables keyed by parent node id.
///
void collectVariables(const QList<NodeDefinition> &nodes, const AddressSpace &addressSpace,
                      QHash<QString, QList<VariableInfo>> &result)
{
    for (const NodeDefinition &node : nodes) {
        if (node.kind == NodeDefinition::Kind::Variable)
            continue;
        QList<VariableInfo> variables;
        for (const NodeDefinition &child : node.children) {
            if (child.kind == NodeDefinition::Kind::Variable && !child.declared)
                variables.append(SnapshotBuilder::variableInfo(child, addressSpace));
        }
        if (!variables.isEmpty())
            result.insert(node.nodeId, variables);
        collectVariables(node.children, addressSpace, result);
    }
}

///
/// \brief Adds the properties and component variables of served variables below them, as OPC UA clients browse
///        them, and lists them as the variables of their variable.
/// \param server Server holding the nodes.
/// \param nodes Tree nodes below the Objects folder.
/// \param variables Variables keyed by the node id of their owner, receiving the children keyed by their variable.
///
void addVariableChildren(QUaServer &server, QList<TreeNode> &nodes, QHash<QString, QList<VariableInfo>> &variables)
{
    for (TreeNode &node : nodes) {
        const auto *variable =
            node.kind == TreeNode::Kind::Variable ? server.nodeById<QUaBaseDataVariable>(QUaNodeId(node.id)) : nullptr;
        if (variable) {
            const QList<QUaNode *> children = variable->browseChildren();
            for (const QUaNode *child : children) {
                const auto *served = qobject_cast<const QUaBaseVariable *>(child);
                if (!served)
                    continue;
                const VariableInfo info = VariableProperties::describeChild(server, *served);
                TreeNode leaf;
                leaf.text = info.displayName;
                leaf.icon = info.icon;
                leaf.id = info.nodeId;
                leaf.kind = qobject_cast<const QUaProperty *>(child) ? TreeNode::Kind::Property
                                                                     : TreeNode::Kind::Variable;
                node.children.append(leaf);
                variables[node.id].append(info);
            }
        }
        addVariableChildren(server, node.children, variables);
    }
}

///
/// \brief Describes a namespace URI.
/// \param index Namespace index.
/// \return Description of the standard and application namespaces; empty otherwise.
///
QString namespaceDescription(int index)
{
    switch (index) {
    case 0:  return QCoreApplication::translate("SnapshotBuilder", "OPC UA standard namespace.");
    case 1:  return QCoreApplication::translate("SnapshotBuilder", "Application namespace of the simulated nodes.");
    default: return QCoreApplication::translate("SnapshotBuilder", "Imported information model.");
    }
}

}

namespace SnapshotBuilder {

///
/// \brief Builds the address space tree of a project.
/// \param nodes Project node tree with node ids.
/// \return Objects, Types and Views roots; the path to the first variable is expanded and
///         that variable is current.
///
QList<TreeNode> addressSpaceTree(const QList<NodeDefinition> &nodes)
{
    TreeNode objects = standardFolder(QStringLiteral("Objects"), QStringLiteral("i=85"));
    objects.expanded = true;
    for (const NodeDefinition &node : nodes)
        objects.children.append(treeNode(node));
    selectFirstVariable(objects.children);

    TreeNode types = standardFolder(QStringLiteral("Types"), QStringLiteral("i=86"));
    types.children = { standardFolder(QStringLiteral("ObjectTypes"), QStringLiteral("i=88")),
                       standardFolder(QStringLiteral("VariableTypes"), QStringLiteral("i=89")) };

    return { objects, types, standardFolder(QStringLiteral("Views"), QStringLiteral("i=87")) };
}

///
/// \brief Describes a variable for the variables table.
/// \param node Variable definition.
/// \param addressSpace Built nodes providing the live value and quality.
/// \return Description; an unbuilt variable shows its initial value with Bad quality.
///
VariableInfo variableInfo(const NodeDefinition &node, const AddressSpace &addressSpace)
{
    const bool built = addressSpace.hasVariable(node.nodeId);
    VariableInfo info;
    info.nodeId = node.nodeId;
    info.displayName = node.browseName;
    info.icon = iconOf(node);
    info.value = built ? addressSpace.value(node.nodeId) : node.initialValue;
    info.dataType = node.dataType;
    info.arrayLength = node.arrayLength;
    info.access = node.access;
    info.quality = built ? addressSpace.quality(node.nodeId) : VariableInfo::Quality::Bad;
    info.initialValue = node.initialValue;
    info.configuredQuality = node.quality;
    info.historizing = node.historizing;
    info.alarm = node.alarm;
    info.alarmState = addressSpace.alarmState(node.nodeId);
    info.simulation = node.simulation;
    info.typeName = addressSpace.typeName(node.nodeId);
    info.properties = addressSpace.properties(node.nodeId);
    return info;
}

///
/// \brief Lists the variables of every folder and object.
/// \param nodes Project node tree with node ids.
/// \param addressSpace Built nodes providing live values and quality.
/// \return Variables keyed by the node id of their parent; unbuilt variables show the
///         initial value with Bad quality.
///
QHash<QString, QList<VariableInfo>> variables(const QList<NodeDefinition> &nodes, const AddressSpace &addressSpace)
{
    QHash<QString, QList<VariableInfo>> result;
    collectVariables(nodes, addressSpace, result);
    return result;
}

///
/// \brief Builds the server summary.
/// \param project Project being served.
/// \param server Server reporting state, start time and clients.
/// \param addressSpace Built nodes.
/// \param now Current time, for the uptime.
/// \return Summary for the status page and the status bar.
///
ServerStatus status(const ProjectConfig &project, const SimServer &server, const AddressSpace &addressSpace,
                    const QDateTime &now)
{
    const ListenerSettings listener = ListenerSettingsBuilder::build(project.endpoints);

    ServerStatus status;
    status.running = server.isRunning();
    status.primaryEndpoint = listener.ok() ? QStringLiteral("opc.tcp://%1:%2").arg(listener.hostname).arg(listener.port)
                                           : QString();
    status.applicationUri = project.server.applicationUri;
    status.securityModes = listener.securityModes;
    status.totalNodes = addressSpace.nodeCount();
    status.namespaces = int(server.uaServer()->namespaces().size());
    status.clientsConnected = server.clientCount();
    if (status.running) {
        status.startTime = server.startTime();
        status.uptimeSeconds = qMax<qint64>(0, server.startTime().secsTo(now));
    }
    return status;
}

///
/// \brief Lists the server namespace array.
/// \param server Server providing the namespace URIs.
/// \return One entry per namespace index.
///
QList<NamespaceInfo> namespaces(const SimServer &server)
{
    QList<NamespaceInfo> result;
    const QStringList uris = server.uaServer()->namespaces();
    for (qsizetype index = 0; index < uris.size(); ++index) {
        NamespaceInfo info;
        info.index = int(index);
        info.uri = uris.at(index);
        info.description = namespaceDescription(info.index);
        result.append(info);
    }
    return result;
}

///
/// \brief Builds everything the main window shows.
/// \param project Project being served.
/// \param server Server reporting state and sessions.
/// \param addressSpace Built nodes.
/// \param now Current time, for the uptime.
/// \return Snapshot without log entries and certificates.
///
ServerSnapshot build(const ProjectConfig &project, const SimServer &server, const AddressSpace &addressSpace,
                     const QDateTime &now)
{
    ServerSnapshot snapshot;
    snapshot.status = status(project, server, addressSpace, now);
    snapshot.addressSpace = addressSpaceTree(project.nodes);
    snapshot.variables = variables(project.nodes, addressSpace);
    DeclaredNodes::collect(*server.uaServer(), project.nodes, addressSpace, snapshot.addressSpace.first().children,
                           snapshot.variables);
    ImportedNodes::collect(*server.uaServer(), ModelVariables::resolve(project.modelVariables, *server.uaServer()),
                           addressSpace, snapshot.addressSpace.first().children, snapshot.variables);
    addVariableChildren(*server.uaServer(), snapshot.addressSpace.first().children, snapshot.variables);
    snapshot.types = TypeCatalog::types(*server.uaServer());
    snapshot.namespaces = namespaces(server);
    snapshot.endpoints = project.endpoints;
    snapshot.sessions = server.sessions();
    return snapshot;
}

}
