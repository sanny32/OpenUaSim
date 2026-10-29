// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

/// \file methodscenario.h
/// \brief Declares validation and execution of declarative method scenarios.

#pragma once

#include "data/methodinfo.h"
#include <QHash>

class QUaServer;
class AddressSpace;

namespace MethodScenario {

///
/// \brief Stores project references by fixed index and imported references by namespace URI.
/// \param nodeId Current server node id.
/// \param namespaces Server namespace URIs.
/// \return Persistent reference, or empty for an invalid node id.
///
QString nodeReference(const QString &nodeId, const QStringList &namespaces);

///
/// \brief Checks sources, target variables and argument compatibility without changing values.
/// \param server Server supplying variable and data type metadata.
/// \param method Signature and scenario to check.
/// \return First validation error, or empty on success.
///
QString problem(QUaServer &server, const MethodInfo &method);

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
                const QVariantList &inputs, QVariantList &outputs, QHash<QString, QVariant> &changes, QString &error);

///
/// \brief Validates names and supported scalar or one-dimensional argument types.
/// \param arguments Arguments on one side of a signature.
/// \return First signature error, or empty on success.
///
QString signatureProblem(const QList<ArgumentInfo> &arguments);

}
