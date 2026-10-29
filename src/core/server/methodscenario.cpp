// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

/// \file methodscenario.cpp
/// \brief Implements typed value sources and ordered variable assignments for methods.

#include <QCoreApplication>
#include <QDateTime>
#include <QRandomGenerator>
#include <QSet>
#include <QUaBaseDataVariable>
#include <QUaServer>

#include "addressspace.h"
#include "methodscenario.h"
#include "modelmethods.h"
#include "project/methodstatuses.h"
#include "project/portablenodeid.h"
#include "simulation/signalgenerator.h"
#include "simulation/valuecast.h"

namespace {

/// \brief Resolves a portable variable reference without allowing standard server variables.
QUaBaseDataVariable *variable(QUaServer &server, const QString &reference)
{
    // Namespace 1 belongs to the project even when the application URI changes on server start.
    const QString id = reference.startsWith(QLatin1String("ns=1;"))
        ? reference : PortableNodeId::toServer(reference, server.namespaces());
    const QUaNodeId nodeId(id);
    return id.isEmpty() || nodeId.namespaceIndex() == 0 ? nullptr : server.nodeById<QUaBaseDataVariable>(nodeId);
}

/// \brief Describes the value shape used to check an assignment against a variable.
ArgumentInfo shape(QUaServer &server, const QUaBaseDataVariable &node)
{
    ArgumentInfo info;
    info.name = node.browseName().name();
    info.valueType = ModelMethods::valueType(server, QUaNodeId(node.dataTypeNodeId()));
    info.dataType = info.valueType;
    info.array = node.valueRank() >= 0;
    info.arrayDimensions = node.arrayDimensions();
    if (info.array)
        info.dataType += QStringLiteral("[]");
    return info;
}

/// \brief Merges legacy output constants with the source selected for one output.
MethodValueSource outputSource(const MethodResponse &response, qsizetype index)
{
    if (index < response.sources.size())
        return response.sources.at(index);
    MethodValueSource source;
    source.value = response.outputs.value(index);
    return source;
}

/// \brief Parses a constant using the same scalar and array rules as the existing method response editor.
QVariant constant(const MethodValueSource &source, const ArgumentInfo &target, QString &error)
{
    MethodResponse response;
    response.outputs = { source.value };
    const QVariantList values = ModelMethods::outputValues({ target }, response, &error);
    return values.isEmpty() ? QVariant() : values.first();
}

/// \brief Rejects sources that cannot supply the declared scalar or array type.
QString sourceProblem(QUaServer &server, const MethodValueSource &source, const ArgumentInfo &target,
                       const QList<ArgumentInfo> &inputs)
{
    if (source.kind == MethodValueSource::Kind::Constant) {
        QString error;
        constant(source, target, error);
        return error;
    }
    if (!ValueCast::isSupported(target.valueType))
        return QCoreApplication::translate("MethodScenario", "Unsupported target type for '%1'").arg(target.name);
    ArgumentInfo supplied;
    switch (source.kind) {
    case MethodValueSource::Kind::Input:
        if (source.inputIndex < 0 || source.inputIndex >= inputs.size())
            return QCoreApplication::translate("MethodScenario", "Input argument %1 does not exist").arg(source.inputIndex + 1);
        supplied = inputs.at(source.inputIndex);
        break;
    case MethodValueSource::Kind::Variable: {
        const auto *node = variable(server, source.variableId);
        if (!node)
            return QCoreApplication::translate("MethodScenario", "Source variable '%1' does not exist").arg(source.variableId);
        supplied = shape(server, *node);
        break;
    }
    case MethodValueSource::Kind::Simulation:
        if (target.array)
            return QCoreApplication::translate("MethodScenario", "Simulation sources require a scalar target");
        if (!SignalGenerator::functions().contains(source.simulation.function)
            || source.simulation.function == SimulationFunction::None
            || source.simulation.intervalMs < 1 || source.simulation.periodMs < 1
            || !qIsFinite(source.simulation.amplitude) || !qIsFinite(source.simulation.offset))
            return QCoreApplication::translate("MethodScenario", "Invalid method simulation settings");
        if (source.simulation.function == SimulationFunction::List) {
            if (source.simulation.values.isEmpty())
                return QCoreApplication::translate("MethodScenario", "Simulation value list is empty");
            for (const QString &text : source.simulation.values) {
                if (!ValueCast::fromText(text, target.valueType).isValid())
                    return QCoreApplication::translate("MethodScenario", "Invalid simulation value '%1'").arg(text);
            }
        }
        return {};
    case MethodValueSource::Kind::Constant:
        break;
    default:
        return QCoreApplication::translate("MethodScenario", "Unknown method value source");
    }
    if (supplied.valueType != target.valueType || supplied.array != target.array
        || supplied.arrayDimensions.size() > 1 || target.arrayDimensions.size() > 1)
        return QCoreApplication::translate("MethodScenario", "Source type does not match '%1' (%2)").arg(target.name, target.dataType);
    return {};
}

/// \brief Samples a validated source at the moment the corresponding action or output executes.
QVariant sample(QUaServer &server, const MethodValueSource &source, const ArgumentInfo &target,
                 const QVariantList &inputs, qint64 elapsedMs, QString &error)
{
    QVariant value;
    switch (source.kind) {
    case MethodValueSource::Kind::Constant:
        return constant(source, target, error);
    case MethodValueSource::Kind::Input:
        if (source.inputIndex >= 0 && source.inputIndex < inputs.size())
            value = inputs.at(source.inputIndex);
        break;
    case MethodValueSource::Kind::Variable:
        if (auto *node = variable(server, source.variableId))
            value = node->value();
        break;
    case MethodValueSource::Kind::Simulation:
        value = SignalGenerator::sample(source.simulation, target.valueType, elapsedMs, *QRandomGenerator::global());
        break;
    }
    if (!value.isValid())
        error = QCoreApplication::translate("MethodScenario", "Method value source is unavailable");
    return error.isEmpty() ? value : QVariant();
}

/// \brief Checks a sampled value against a declared fixed one-dimensional array size.
bool matchesDimensions(const QVariant &value, const ArgumentInfo &target)
{
    return !target.array || !value.isValid() || target.arrayDimensions.isEmpty()
        || (target.arrayDimensions.size() == 1
            && (target.arrayDimensions.first() == 0 || value.toList().size() == target.arrayDimensions.first()));
}

}

namespace MethodScenario {

///
/// \brief Stores project references by fixed index and imported references by namespace URI.
/// \param nodeId Current server node id.
/// \param namespaces Server namespace URIs.
/// \return Persistent reference, or empty for an invalid node id.
///
QString nodeReference(const QString &nodeId, const QStringList &namespaces)
{
    const QString portable = PortableNodeId::fromServer(nodeId, namespaces);
    return !portable.isEmpty() && nodeId.startsWith(QLatin1String("ns=1;")) ? nodeId : portable;
}

///
/// \brief Validates names and supported scalar or one-dimensional argument types.
/// \param arguments Arguments on one side of a signature.
/// \return First signature error, or empty on success.
///
QString signatureProblem(const QList<ArgumentInfo> &arguments)
{
    QSet<QString> names;
    for (const ArgumentInfo &argument : arguments) {
        if (argument.name.trimmed().isEmpty() || names.contains(argument.name))
            return QCoreApplication::translate("MethodScenario", "Argument names must be nonempty and unique");
        if (!ValueCast::isSupported(argument.valueType))
            return QCoreApplication::translate("MethodScenario", "Unsupported argument type '%1'").arg(argument.valueType);
        names.insert(argument.name);
    }
    return {};
}

///
/// \brief Checks sources, target variables and argument compatibility without changing values.
/// \param server Server supplying variable and data type metadata.
/// \param method Signature and scenario to check.
/// \return First validation error, or empty on success.
///
QString problem(QUaServer &server, const MethodInfo &method)
{
    if (MethodStatuses::name(method.response.statusCode).isEmpty())
        return QCoreApplication::translate("MethodScenario", "Unknown method status code");
    if (method.response.sources.size() > method.outputs.size())
        return QCoreApplication::translate("MethodScenario", "More output sources than output arguments");
    for (qsizetype index = 0; index < method.outputs.size(); ++index) {
        const QString error = sourceProblem(server, outputSource(method.response, index), method.outputs.at(index), method.inputs);
        if (!error.isEmpty())
            return error;
    }
    for (const MethodSetAction &action : method.response.actions) {
        if (!action.enabled)
            continue;
        const auto *node = variable(server, action.targetId);
        if (!node)
            return QCoreApplication::translate("MethodScenario", "Target variable '%1' does not exist").arg(action.targetId);
        const ArgumentInfo target = shape(server, *node);
        if (!ValueCast::isSupported(target.valueType))
            return QCoreApplication::translate("MethodScenario", "Unsupported target type for '%1'").arg(target.name);
        const QString error = sourceProblem(server, action.source, target, method.inputs);
        if (!error.isEmpty())
            return error;
    }
    return {};
}

///
/// \brief Executes enabled assignments in order and then samples output sources.
/// \param server Server holding source and target variables.
/// \param addressSpace Preserves quality and alarm handling for managed variables; may be null.
/// \param method Signature and scenario to execute.
/// \param inputs Values supplied by the caller.
/// \param outputs Receives sampled output arguments only on success.
/// \param changes Receives successful assignments, including those preceding a runtime error.
/// \param error Receives a runtime error, or empty on success.
/// \return Configured status or an execution error; a configured Bad status performs no assignments.
///
quint32 execute(QUaServer &server, AddressSpace *addressSpace, const MethodInfo &method,
                const QVariantList &inputs, QVariantList &outputs, QHash<QString, QVariant> &changes, QString &error)
{
    outputs.clear();
    changes.clear();
    error = problem(server, method);
    if (!error.isEmpty())
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    if (method.response.statusCode & 0x80000000u)
        return method.response.statusCode;
    if (inputs.size() != method.inputs.size()) {
        error = QCoreApplication::translate("MethodScenario", "Method input count does not match its signature");
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    }
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (!server.property("ouasimMethodEpoch").isValid())
        server.setProperty("ouasimMethodEpoch", now);
    const qint64 elapsed = now - server.property("ouasimMethodEpoch").toLongLong();
    for (const MethodSetAction &action : method.response.actions) {
        if (!action.enabled)
            continue;
        auto *node = variable(server, action.targetId);
        const ArgumentInfo target = shape(server, *node);
        QVariant value = sample(server, action.source, target, inputs, elapsed, error);
        if (!error.isEmpty())
            return UA_STATUSCODE_BADCONFIGURATIONERROR;
        if (!value.isValid())
            value = target.array ? QVariant(QVariantList()) : ValueCast::defaultValue(target.valueType);
        if (!matchesDimensions(value, target)) {
            error = QCoreApplication::translate("MethodScenario", "Array length does not match '%1'").arg(target.name);
            return UA_STATUSCODE_BADTYPEMISMATCH;
        }
        const QString nodeId = node->nodeId().toXmlString();
        if (!addressSpace || !addressSpace->hasVariable(nodeId))
            node->setValue(value, node->statusCode(), QDateTime(), QDateTime(), ValueCast::metaType(target.valueType));
        else
            addressSpace->setValue(nodeId, value);
        changes.insert(nodeId, node->value());
    }
    QVariantList result;
    for (qsizetype index = 0; index < method.outputs.size(); ++index) {
        const ArgumentInfo &output = method.outputs.at(index);
        const QVariant value = sample(server, outputSource(method.response, index), output, inputs, elapsed, error);
        if (!error.isEmpty())
            return UA_STATUSCODE_BADCONFIGURATIONERROR;
        if (!matchesDimensions(value, output)) {
            error = QCoreApplication::translate("MethodScenario", "Array length does not match '%1'").arg(output.name);
            return UA_STATUSCODE_BADTYPEMISMATCH;
        }
        result.append(value);
    }
    outputs = result;
    return method.response.statusCode;
}

}
