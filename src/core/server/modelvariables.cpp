// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file modelvariables.cpp
/// \brief Implements the mapping between the project settings of model variables and the server nodes.
///

#include <QCoreApplication>

#include <QUaServer>

#include "importednodes.h"
#include "modelvariables.h"
#include "simulation/valuecast.h"

namespace {

constexpr quint16 kFirstModelNamespace = 2;

///
/// \brief Translates a text of this file.
/// \param text Source text.
/// \return Translated text.
///
QString tr(const char *text)
{
    return QCoreApplication::translate("ModelVariables", text);
}

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
/// \brief Returns a node id without its namespace index.
/// \param nodeId Node id of any namespace.
/// \return Identifier such as "i=1003".
///
QString identifierOf(QUaNodeId nodeId)
{
    nodeId.setNamespaceIndex(0);
    QString identifier = nodeId.toXmlString();
    if (identifier.startsWith(QLatin1String("ns=0;")))
        identifier.remove(0, 5);
    return identifier;
}

}

namespace ModelVariables {

///
/// \brief Returns the variable of an imported model that a server node id names.
/// \param server Server holding the models.
/// \param nodeId Node id with a server namespace index, e.g. "ns=2;i=1003".
/// \return Variable, or null for an unknown node or a node of namespace 0 or 1.
///
QUaBaseDataVariable *find(QUaServer &server, const QString &nodeId)
{
    const QUaNodeId id(nodeId);
    if (id.isNull() || id.namespaceIndex() < kFirstModelNamespace)
        return nullptr;
    return server.nodeById<QUaBaseDataVariable>(id);
}

///
/// \brief Describes a model variable as it is defined by its model.
/// \param server Server holding the models.
/// \param variable Variable of an imported model.
/// \return Settings with the namespace URI, the identifier within it, the browse name, data type, array
///         length and access of the variable, Good quality and an invalid initial value.
///
ModelVariable describe(const QUaServer &server, const QUaBaseDataVariable &variable)
{
    const QUaNodeId nodeId = variable.nodeId();
    ModelVariable result;
    result.namespaceUri = server.namespaces().value(nodeId.namespaceIndex());
    NodeDefinition &definition = result.variable;
    definition.kind = NodeDefinition::Kind::Variable;
    definition.nodeId = identifierOf(nodeId);
    definition.browseName = variable.browseName().name();
    definition.icon = QStringLiteral("signal");
    definition.dataType = ImportedNodes::dataTypeName(variable.dataTypeNodeId());
    const QVariant value = variable.value();
    definition.arrayLength = ValueCast::isArray(value) ? int(value.toList().size()) : 0;
    definition.access = accessText(variable);
    definition.initialValue = QVariant();
    return result;
}

///
/// \brief Tells whether the simulation can drive a model variable.
/// \param variable Variable of an imported model.
/// \return True for a scalar or one-dimensional array of a supported built-in data type.
///
bool isEditable(const QUaBaseDataVariable &variable)
{
    const qint32 rank = variable.valueRank();
    if (rank != UA_VALUERANK_SCALAR && rank != UA_VALUERANK_ONE_DIMENSION)
        return false;
    const QVariant value = variable.value();
    return ValueCast::isSupported(ImportedNodes::dataTypeName(variable.dataTypeNodeId()))
        && (!ValueCast::isArray(value) || value.toList().size() <= kMaxArrayLength);
}

///
/// \brief Finds the stored settings of a model variable.
/// \param variables Settings kept by the project.
/// \param namespaceUri URI of the model namespace.
/// \param identifier Node id within the namespace, e.g. "i=1003".
/// \return Index into variables, or -1 when the project keeps none.
///
qsizetype indexOf(const QList<ModelVariable> &variables, const QString &namespaceUri, const QString &identifier)
{
    for (qsizetype index = 0; index < variables.size(); ++index) {
        const ModelVariable &variable = variables.at(index);
        if (variable.namespaceUri == namespaceUri && variable.variable.nodeId == identifier)
            return index;
    }
    return -1;
}

///
/// \brief Turns the stored settings into variable definitions of the served model nodes.
/// \param variables Settings kept by the project.
/// \param server Server holding the models.
/// \param problems Receives why settings were skipped: an unloaded namespace, an unknown node or a variable
///        the simulation cannot drive; may be null.
/// \return Definitions with server node ids, and the browse names, data types and array lengths of the model.
///
QList<NodeDefinition> resolve(const QList<ModelVariable> &variables, QUaServer &server, QStringList *problems)
{
    const QStringList namespaces = server.namespaces();
    QList<NodeDefinition> result;
    for (const ModelVariable &stored : variables) {
        const QString path = QStringLiteral("%1;%2").arg(stored.namespaceUri, stored.variable.nodeId);
        const qsizetype namespaceIndex = namespaces.indexOf(stored.namespaceUri);
        QUaNodeId nodeId(stored.variable.nodeId);
        QUaBaseDataVariable *variable = nullptr;
        if (namespaceIndex >= kFirstModelNamespace && !nodeId.isNull()) {
            nodeId.setNamespaceIndex(quint16(namespaceIndex));
            variable = find(server, nodeId.toXmlString());
        }
        QString problem;
        if (namespaceIndex < kFirstModelNamespace)
            problem = tr("no imported model has the namespace");
        else if (!variable)
            problem = tr("the model has no such variable");
        else if (!isEditable(*variable))
            problem = tr("the simulation cannot drive a %1 variable")
                          .arg(ImportedNodes::dataTypeName(variable->dataTypeNodeId()));
        if (!problem.isEmpty()) {
            if (problems)
                problems->append(QStringLiteral("%1: %2").arg(path, problem));
            continue;
        }
        const NodeDefinition model = describe(server, *variable).variable;
        NodeDefinition definition = stored.variable;
        definition.nodeId = nodeId.toXmlString();
        definition.browseName = model.browseName;
        definition.dataType = model.dataType;
        definition.arrayLength = model.arrayLength;
        result.append(definition);
    }
    return result;
}

}
