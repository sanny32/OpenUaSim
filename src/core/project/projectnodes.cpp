// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectnodes.cpp
/// \brief Implements helpers over the node tree of a project.
///

#include <QCoreApplication>
#include <QSet>

#include "alarmrules.h"
#include "projectnodes.h"
#include "simulation/valuecast.h"

namespace {

///
/// \brief Translates a message of the node tree helpers.
/// \param text Source text.
/// \return Translated text.
///
QString tr(const char *text)
{
    return QCoreApplication::translate("ProjectNodes", text);
}

///
/// \brief Stores a failure reason when the caller asked for one.
/// \param error Destination; may be null.
/// \param message Reason.
///
void setError(QString *error, const QString &message)
{
    if (error)
        *error = message;
}

///
/// \brief Finds a node by id for modification.
/// \param nodes Node tree.
/// \param nodeId Node id to look for.
/// \return Node, or null.
///
NodeDefinition *findMutable(QList<NodeDefinition> &nodes, const QString &nodeId)
{
    for (NodeDefinition &node : nodes) {
        if (node.nodeId == nodeId)
            return &node;
        if (NodeDefinition *found = findMutable(node.children, nodeId))
            return found;
    }
    return nullptr;
}

///
/// \brief Finds the sibling list a node belongs to.
/// \param nodes Node tree.
/// \param nodeId Node id to look for.
/// \return List holding the node, or null.
///
QList<NodeDefinition> *siblingsOf(QList<NodeDefinition> &nodes, const QString &nodeId)
{
    for (NodeDefinition &node : nodes) {
        if (node.nodeId == nodeId)
            return &nodes;
        if (QList<NodeDefinition> *found = siblingsOf(node.children, nodeId))
            return found;
    }
    return nullptr;
}

///
/// \brief Returns the children list of a container.
/// \param nodes Node tree.
/// \param parentId Folder or object id; empty for the top level.
/// \return Children list, or null when the parent is unknown or is a variable or method.
///
QList<NodeDefinition> *childrenOf(QList<NodeDefinition> &nodes, const QString &parentId)
{
    if (parentId.isEmpty())
        return &nodes;
    NodeDefinition *parent = findMutable(nodes, parentId);
    if (!parent || parent->kind == NodeDefinition::Kind::Variable || parent->kind == NodeDefinition::Kind::Method)
        return nullptr;
    return &parent->children;
}

///
/// \brief Tells whether a sibling other than a node already has a browse name.
/// \param siblings Sibling list.
/// \param browseName Name to look for.
/// \param exceptNodeId Node to ignore; empty ignores none.
/// \return True when the name is taken.
///
bool nameTaken(const QList<NodeDefinition> &siblings, const QString &browseName, const QString &exceptNodeId)
{
    for (const NodeDefinition &sibling : siblings) {
        if (sibling.browseName == browseName && (exceptNodeId.isEmpty() || sibling.nodeId != exceptNodeId))
            return true;
    }
    return false;
}

///
/// \brief Checks a new browse name of an existing node.
/// \param nodes Node tree holding the node.
/// \param nodeId Node to rename.
/// \param browseName Proposed name.
/// \param error Receives the reason on failure; may be null.
/// \return False when the name is empty or used by a sibling.
///
bool checkRename(QList<NodeDefinition> &nodes, const QString &nodeId, const QString &browseName, QString *error)
{
    if (browseName.trimmed().isEmpty()) {
        setError(error, tr("The browse name is empty"));
        return false;
    }
    if (nameTaken(*siblingsOf(nodes, nodeId), browseName, nodeId)) {
        setError(error, tr("A sibling is already named '%1'").arg(browseName));
        return false;
    }
    return true;
}

///
/// \brief Drops the node ids of a node and its children so new ones get assigned.
/// \param node Node to clear.
///
void clearNodeIds(NodeDefinition &node)
{
    node.nodeId.clear();
    for (NodeDefinition &child : node.children)
        clearNodeIds(child);
}

///
/// \brief Collects the node ids already present in a tree.
/// \param nodes Node tree.
/// \param ids Receives the non-empty ids.
///
void collectIds(const QList<NodeDefinition> &nodes, QSet<QString> &ids)
{
    for (const NodeDefinition &node : nodes) {
        if (!node.nodeId.isEmpty())
            ids.insert(node.nodeId);
        collectIds(node.children, ids);
    }
}

///
/// \brief Assigns ids below one parent.
/// \param nodes Siblings to complete.
/// \param parentPath Dotted browse path of their parent; empty at the root.
/// \param used Ids already taken; extended with the assigned ones.
/// \return Number of ids assigned.
///
int assign(QList<NodeDefinition> &nodes, const QString &parentPath, QSet<QString> &used)
{
    int assigned = 0;
    for (NodeDefinition &node : nodes) {
        const QString path = parentPath.isEmpty() ? node.browseName
                                                  : parentPath + QLatin1Char('.') + node.browseName;
        if (node.nodeId.isEmpty()) {
            QString candidate = QStringLiteral("ns=1;s=%1").arg(path);
            for (int suffix = 2; used.contains(candidate); ++suffix)
                candidate = QStringLiteral("ns=1;s=%1_%2").arg(path).arg(suffix);
            node.nodeId = candidate;
            used.insert(candidate);
            ++assigned;
        }
        assigned += assign(node.children, path, used);
    }
    return assigned;
}

}

namespace ProjectNodes {

///
/// \brief Gives every node without a node id one derived from its browse path.
/// \param nodes Node tree to complete in place.
/// \return Number of node ids assigned; ids such as "ns=1;s=Line.Motor.Speed" get a numeric
///         suffix when the path is already taken.
///
int assignMissingNodeIds(QList<NodeDefinition> &nodes)
{
    QSet<QString> used;
    collectIds(nodes, used);
    return assign(nodes, QString(), used);
}

///
/// \brief Counts the nodes of a tree.
/// \param nodes Node tree.
/// \return Number of nodes at every depth.
///
int count(const QList<NodeDefinition> &nodes)
{
    int total = 0;
    for (const NodeDefinition &node : nodes)
        total += 1 + count(node.children);
    return total;
}

///
/// \brief Finds a node by id.
/// \param nodes Node tree.
/// \param nodeId Node id to look for.
/// \return Node, or null when no node has that id.
///
const NodeDefinition *find(const QList<NodeDefinition> &nodes, const QString &nodeId)
{
    return findMutable(const_cast<QList<NodeDefinition> &>(nodes), nodeId);
}

///
/// \brief Returns the node ids used in a tree.
/// \param nodes Node tree.
/// \return Non-empty node ids.
///
QSet<QString> nodeIds(const QList<NodeDefinition> &nodes)
{
    QSet<QString> ids;
    collectIds(nodes, ids);
    return ids;
}

///
/// \brief Returns the browse names of the children of a container.
/// \param nodes Node tree.
/// \param parentId Folder or object id; empty for the top level.
/// \return Browse names, or empty when the parent is unknown.
///
QStringList childNames(const QList<NodeDefinition> &nodes, const QString &parentId)
{
    QStringList names;
    if (const QList<NodeDefinition> *children = childrenOf(const_cast<QList<NodeDefinition> &>(nodes), parentId)) {
        for (const NodeDefinition &child : *children)
            names.append(child.browseName);
    }
    return names;
}

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
QString insert(QList<NodeDefinition> &nodes, const QString &parentId, const NodeDefinition &node, QString *error)
{
    QList<NodeDefinition> *siblings = childrenOf(nodes, parentId);
    if (!siblings) {
        setError(error, tr("'%1' cannot hold child nodes").arg(parentId));
        return {};
    }
    if (node.browseName.trimmed().isEmpty()) {
        setError(error, tr("The browse name is empty"));
        return {};
    }
    if (node.kind == NodeDefinition::Kind::Method && parentId.isEmpty()) {
        setError(error, tr("A method belongs to a folder or object"));
        return {};
    }
    if (nameTaken(*siblings, node.browseName, QString())) {
        setError(error, tr("A sibling is already named '%1'").arg(node.browseName));
        return {};
    }
    if (!node.nodeId.isEmpty() && find(nodes, node.nodeId)) {
        setError(error, tr("The node id '%1' is already used").arg(node.nodeId));
        return {};
    }
    if (node.kind == NodeDefinition::Kind::Variable && !ValueCast::isSupported(node.dataType)) {
        setError(error, tr("Unsupported data type '%1'").arg(node.dataType));
        return {};
    }
    if (node.arrayLength < 0 || node.arrayLength > kMaxArrayLength) {
        setError(error, tr("The array length must be between 0 and %1").arg(kMaxArrayLength));
        return {};
    }
    if (const QString problem = AlarmRules::problem(node.alarm, node.dataType, node.arrayLength); !problem.isEmpty()) {
        setError(error, problem);
        return {};
    }

    const qsizetype index = siblings->size();
    siblings->append(node);
    assignMissingNodeIds(nodes);
    return childrenOf(nodes, parentId)->at(index).nodeId;
}

///
/// \brief Removes a node with its children.
/// \param nodes Node tree to shrink.
/// \param nodeId Node to remove.
/// \return False when no node has that id.
///
bool remove(QList<NodeDefinition> &nodes, const QString &nodeId)
{
    QList<NodeDefinition> *siblings = siblingsOf(nodes, nodeId);
    if (!siblings)
        return false;
    for (qsizetype index = 0; index < siblings->size(); ++index) {
        if (siblings->at(index).nodeId == nodeId) {
            siblings->removeAt(index);
            return true;
        }
    }
    return false;
}

///
/// \brief Returns the folder or object holding a node.
/// \param nodes Node tree.
/// \param nodeId Node to look up.
/// \return Parent node id; empty for top-level and unknown nodes.
///
QString parentId(const QList<NodeDefinition> &nodes, const QString &nodeId)
{
    for (const NodeDefinition &node : nodes) {
        for (const NodeDefinition &child : node.children) {
            if (child.nodeId == nodeId)
                return node.nodeId;
        }
        const QString found = parentId(node.children, nodeId);
        if (!found.isEmpty())
            return found;
    }
    return {};
}

///
/// \brief Copies a node with its children for inserting it elsewhere.
/// \param node Node to copy.
/// \param takenNames Browse names already used where the copy goes.
/// \return Copy without node ids, renamed to "<name>_copy", "<name>_copy2", ... when its name is taken.
///
NodeDefinition detachedCopy(const NodeDefinition &node, const QStringList &takenNames)
{
    NodeDefinition copy = node;
    clearNodeIds(copy);
    if (!takenNames.contains(copy.browseName))
        return copy;
    const QString base = node.browseName + QStringLiteral("_copy");
    copy.browseName = base;
    for (int suffix = 2; takenNames.contains(copy.browseName); ++suffix)
        copy.browseName = base + QString::number(suffix);
    return copy;
}

///
/// \brief Replaces the editable fields of a variable.
/// \param nodes Node tree holding the variable.
/// \param variable Variable with its node id and the new browse name, icon, data type, access,
///        array length, initial value, quality, historizing, simulation, alarm and property values; the initial
///        value is converted to the data type.
/// \param error Receives the reason on failure; may be null.
/// \return False when the node is unknown or not a variable, the browse name is empty or used
///         by a sibling, the data type is unsupported, the array length is outside 0..kMaxArrayLength, or
///         the alarm settings are refused by AlarmRules::problem().
///
bool updateVariable(QList<NodeDefinition> &nodes, const NodeDefinition &variable, QString *error)
{
    NodeDefinition *existing = findMutable(nodes, variable.nodeId);
    if (!existing || existing->kind != NodeDefinition::Kind::Variable) {
        setError(error, tr("'%1' is not a variable").arg(variable.nodeId));
        return false;
    }
    if (!checkRename(nodes, variable.nodeId, variable.browseName, error))
        return false;
    if (!ValueCast::isSupported(variable.dataType)) {
        setError(error, tr("Unsupported data type '%1'").arg(variable.dataType));
        return false;
    }
    if (variable.arrayLength < 0 || variable.arrayLength > kMaxArrayLength) {
        setError(error, tr("The array length must be between 0 and %1").arg(kMaxArrayLength));
        return false;
    }
    if (const QString problem = AlarmRules::problem(variable.alarm, variable.dataType, variable.arrayLength);
        !problem.isEmpty()) {
        setError(error, problem);
        return false;
    }

    existing->alarm = variable.alarm;
    existing->arrayLength = variable.arrayLength;
    existing->initialValue = ValueCast::convert(variable.initialValue, variable.dataType);
    existing->browseName = variable.browseName;
    existing->icon = variable.icon;
    existing->dataType = variable.dataType;
    existing->access = variable.access;
    existing->quality = variable.quality;
    existing->historizing = variable.historizing;
    existing->simulation = variable.simulation;
    existing->properties = variable.properties;
    return true;
}

///
/// \brief Gives a folder, object or variable a new browse name; its node id is kept.
/// \param nodes Node tree holding the node.
/// \param nodeId Node to rename.
/// \param browseName New name.
/// \param error Receives the reason on failure; may be null.
/// \return False when the node is unknown or the name is empty or used by a sibling.
///
bool rename(QList<NodeDefinition> &nodes, const QString &nodeId, const QString &browseName, QString *error)
{
    NodeDefinition *existing = findMutable(nodes, nodeId);
    if (!existing) {
        setError(error, tr("There is no node '%1'").arg(nodeId));
        return false;
    }
    if (!checkRename(nodes, nodeId, browseName, error))
        return false;
    existing->browseName = browseName;
    return true;
}

}
