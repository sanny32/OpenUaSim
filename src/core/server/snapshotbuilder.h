// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file snapshotbuilder.h
/// \brief Declares the assembly of the state the UI shows from the project and the running server.
///

#pragma once

#include <QDateTime>
#include <QHash>
#include <QList>

#include "data/projectconfig.h"
#include "data/serversnapshot.h"

class AddressSpace;
class QUaServer;
class SimServer;

namespace SnapshotBuilder {

///
/// \brief Builds the address space tree of a project.
/// \param nodes Project node tree with node ids.
/// \return Objects, Types and Views roots; the path to the first variable is expanded and
///         that variable is current.
///
QList<TreeNode> addressSpaceTree(const QList<NodeDefinition> &nodes);

///
/// \brief Describes a variable for the variables table.
/// \param node Variable definition.
/// \param addressSpace Built nodes providing the live value and quality.
/// \return Description; an unbuilt variable shows its initial value with Bad quality.
///
VariableInfo variableInfo(const NodeDefinition &node, const AddressSpace &addressSpace);

///
/// \brief Lists the variables of every folder and object.
/// \param nodes Project node tree with node ids.
/// \param addressSpace Built nodes providing live values and quality.
/// \return Variables keyed by the node id of their parent; unbuilt variables show the
///         initial value with Bad quality.
///
QHash<QString, QList<VariableInfo>> variables(const QList<NodeDefinition> &nodes, const AddressSpace &addressSpace);

///
/// \brief Builds the server summary.
/// \param project Project being served.
/// \param server Server reporting state, start time and clients.
/// \param addressSpace Built nodes.
/// \param now Current time, for the uptime.
/// \return Summary for the status page and the status bar.
///
ServerStatus status(const ProjectConfig &project, const SimServer &server, const AddressSpace &addressSpace,
                    const QDateTime &now);

///
/// \brief Lists the server namespace array.
/// \param server Server providing the namespace URIs.
/// \return One entry per namespace index.
///
QList<NamespaceInfo> namespaces(const SimServer &server);

///
/// \brief Describes the methods of an address space tree.
/// \param tree Address space tree.
/// \param project Project being served, providing the actions of its methods and the answers of model methods.
/// \param server Server holding the methods.
/// \return Methods keyed by node id.
///
QHash<QString, MethodInfo> methods(const QList<TreeNode> &tree, const ProjectConfig &project, const QUaServer &server);

///
/// \brief Builds everything the main window shows.
/// \param project Project being served.
/// \param server Server reporting state and sessions.
/// \param addressSpace Built nodes.
/// \param now Current time, for the uptime.
/// \return Snapshot without log entries and certificates.
///
ServerSnapshot build(const ProjectConfig &project, const SimServer &server, const AddressSpace &addressSpace,
                     const QDateTime &now);

}
