// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspace.cpp
/// \brief Implements the project nodes built under the Objects folder of a QUaServer.
///

#include <QUaServer>

#include "addressspace.h"
#include "simulation/valuecast.h"

namespace {

constexpr quint16 kApplicationNamespace = 1;

///
/// \brief Applies an access level text to a variable.
/// \param variable Variable to restrict.
/// \param access "Read", "Write" or "Read/Write".
///
void applyAccess(QUaBaseDataVariable *variable, const QString &access)
{
    variable->setReadAccess(access != QLatin1String("Write"));
    variable->setWriteAccess(access != QLatin1String("Read"));
}

///
/// \brief Sets a variable's value with the Qt type of a data type.
/// \param variable Variable to update.
/// \param value Value to convert.
/// \param dataType Built-in data type name.
///
void setTypedValue(QUaBaseDataVariable *variable, const QVariant &value, const QString &dataType)
{
    variable->setValue(ValueCast::convert(value, dataType), QUaStatus::Good, QDateTime(), QDateTime(),
                       ValueCast::metaType(dataType));
}

}

///
/// \brief Creates an empty address space on a server.
/// \param server Server that receives the nodes; must outlive this object.
/// \param parent Owning QObject.
///
AddressSpace::AddressSpace(QUaServer *server, QObject *parent)
    : QObject(parent)
    , _server(server)
{
}

///
/// \brief Removes the built nodes from the server.
///
AddressSpace::~AddressSpace()
{
    clear();
}

///
/// \brief Replaces the built nodes with a node tree.
/// \param nodes Tree to build under the Objects folder; nodes need node ids.
/// \return Problems with nodes that were skipped together with their children: a missing,
///         malformed or already used node id, an unknown namespace, a duplicated browse
///         name or an unsupported data type.
///
QStringList AddressSpace::build(const QList<NodeDefinition> &nodes)
{
    clear();
    QStringList problems;
    for (const NodeDefinition &node : nodes) {
        if (QUaNode *root = buildNode(_server->objectsFolder(), node, node.browseName, problems))
            _roots.append(root);
    }
    return problems;
}

///
/// \brief Removes every built node from the server.
///
void AddressSpace::clear()
{
    for (const QPointer<QUaNode> &root : std::as_const(_roots))
        delete root.data();
    _roots.clear();
    _variables.clear();
    _dataTypes.clear();
    _variableIds.clear();
    _nodeCount = 0;
}

///
/// \brief Returns the number of built nodes.
/// \return Folders, objects and variables built by the last build().
///
int AddressSpace::nodeCount() const
{
    return _nodeCount;
}

///
/// \brief Tells whether a variable was built.
/// \param nodeId Variable node id.
/// \return True when the variable exists.
///
bool AddressSpace::hasVariable(const QString &nodeId) const
{
    return !_variables.value(nodeId).isNull();
}

///
/// \brief Returns the current value of a variable.
/// \param nodeId Variable node id.
/// \return Value, or an invalid variant for an unknown variable.
///
QVariant AddressSpace::value(const QString &nodeId) const
{
    const QUaBaseDataVariable *variable = _variables.value(nodeId);
    return variable ? variable->value() : QVariant();
}

///
/// \brief Returns the quality of a variable's value.
/// \param nodeId Variable node id.
/// \return Good, Uncertain or Bad, from the severity of the status code; Bad for an unknown variable.
///
VariableInfo::Quality AddressSpace::quality(const QString &nodeId) const
{
    const QUaBaseDataVariable *variable = _variables.value(nodeId);
    if (!variable)
        return VariableInfo::Quality::Bad;
    // Bits 30-31 of a StatusCode hold its severity (OPC UA Part 4, 7.39).
    switch (UA_StatusCode(variable->statusCode()) >> 30) {
    case 0:  return VariableInfo::Quality::Good;
    case 1:  return VariableInfo::Quality::Uncertain;
    default: return VariableInfo::Quality::Bad;
    }
}

///
/// \brief Sets the value of a variable from the server side, keeping its data type.
/// \param nodeId Variable node id.
/// \param value New value, converted to the variable's data type.
/// \return False for an unknown variable.
///
bool AddressSpace::setValue(const QString &nodeId, const QVariant &value)
{
    QUaBaseDataVariable *variable = _variables.value(nodeId);
    if (!variable)
        return false;
    setTypedValue(variable, value, _dataTypes.value(nodeId));
    return true;
}

///
/// \brief Builds one node and, when that succeeds, its children.
/// \param parent Server node receiving the new node.
/// \param node Definition to build.
/// \param path Slash-separated browse path for problem messages.
/// \param problems Receives the reason the node was skipped.
/// \return Created node, or null when it was skipped.
///
QUaNode *AddressSpace::buildNode(QUaNode *parent, const NodeDefinition &node, const QString &path,
                                 QStringList &problems)
{
    const QString problem = checkNode(parent, node);
    if (!problem.isEmpty()) {
        problems.append(QStringLiteral("%1: %2").arg(path, problem));
        return nullptr;
    }

    const QUaQualifiedName browseName(kApplicationNamespace, node.browseName);
    const QUaNodeId nodeId(node.nodeId);
    QUaNode *created = nullptr;
    switch (node.kind) {
    case NodeDefinition::Kind::Folder:
        created = parent->addFolderObject(browseName, nodeId);
        break;
    case NodeDefinition::Kind::Object:
        created = parent->addBaseObject(browseName, nodeId);
        break;
    case NodeDefinition::Kind::Variable:
        created = addVariable(parent, node);
        break;
    }
    if (!created)
        return nullptr;
    ++_nodeCount;
    for (const NodeDefinition &child : node.children)
        buildNode(created, child, path + QLatin1Char('/') + child.browseName, problems);
    return created;
}

///
/// \brief Finds what would make QUaServer refuse a node.
/// \param parent Server node that would receive it.
/// \param node Definition to check.
/// \return Problem text, or empty when the node can be built.
///
QString AddressSpace::checkNode(QUaNode *parent, const NodeDefinition &node) const
{
    if (node.browseName.isEmpty())
        return tr("browse name is empty");
    if (node.nodeId.isEmpty())
        return tr("node id is empty");
    const QUaNodeId nodeId(node.nodeId);
    if (nodeId.isNull())
        return tr("node id '%1' is malformed").arg(node.nodeId);
    if (nodeId.namespaceIndex() >= _server->namespaces().size())
        return tr("namespace %1 of node id '%2' is not registered").arg(nodeId.namespaceIndex()).arg(node.nodeId);
    if (_server->isNodeIdUsed(nodeId))
        return tr("node id '%1' is already used").arg(node.nodeId);
    if (parent->hasChild(QUaQualifiedName(kApplicationNamespace, node.browseName)))
        return tr("browse name '%1' is used by a sibling").arg(node.browseName);
    if (node.kind == NodeDefinition::Kind::Variable && !ValueCast::isSupported(node.dataType))
        return tr("unsupported data type '%1'").arg(node.dataType);
    return {};
}

///
/// \brief Creates a variable with its data type, access level and initial value.
/// \param parent Server node receiving the variable.
/// \param node Variable definition, already checked.
/// \return Created variable, or null when the server refused it.
///
QUaBaseDataVariable *AddressSpace::addVariable(QUaNode *parent, const NodeDefinition &node)
{
    QUaBaseDataVariable *variable =
        parent->addBaseDataVariable(QUaQualifiedName(kApplicationNamespace, node.browseName), QUaNodeId(node.nodeId));
    if (!variable)
        return nullptr;
    setTypedValue(variable, node.initialValue, node.dataType);
    applyAccess(variable, node.access);
    _variables.insert(node.nodeId, variable);
    _dataTypes.insert(node.nodeId, node.dataType);
    _variableIds.insert(variable, node.nodeId);
    connect(variable, &QUaBaseVariable::valueChanged, this, &AddressSpace::handleValueChanged);
    return variable;
}

///
/// \brief Reports values written by clients.
/// \param value New value.
/// \param networkChange True when a client wrote it.
///
void AddressSpace::handleValueChanged(const QVariant &value, const bool &networkChange)
{
    if (networkChange)
        emit valueWrittenByClient(_variableIds.value(sender()), value);
}
