// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectconfig.h
/// \brief Declares the simulation project: server settings, endpoints and the address space to build.
///

#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "endpointinfo.h"
#include "variableinfo.h"

///
/// \brief One node the project adds under the server Objects folder, together with its children.
///
struct NodeDefinition
{
    ///
    /// \brief OPC UA node the definition becomes.
    ///
    enum class Kind { Folder, Object, Variable };

    /// \brief Node class to create.
    Kind kind = Kind::Folder;
    /// \brief Browse and display name.
    QString browseName;
    /// \brief String form of the requested node identifier; empty lets the server assign one.
    QString nodeId;
    /// \brief Resource name of the icon shown in the address space tree.
    QString icon;
    /// \brief Built-in data type name; variables only.
    QString dataType = QStringLiteral("Double");
    /// \brief Access level text: "Read", "Write" or "Read/Write"; variables only.
    QString access = QStringLiteral("Read/Write");
    /// \brief Value the variable starts with; variables only.
    QVariant initialValue;
    /// \brief Simulation driving the value; variables only.
    SimulationSettings simulation;
    /// \brief Child nodes; folders and objects only.
    QList<NodeDefinition> children;
};

///
/// \brief Server description shared by every endpoint.
///
struct ServerSettings
{
    /// \brief Application name announced to clients.
    QString applicationName = QStringLiteral("OpenUaSim");
    /// \brief Application URI announced to clients.
    QString applicationUri = QStringLiteral("urn:openuasim:server");
    /// \brief Maximum number of concurrent sessions.
    int maxSessions = 100;
};

///
/// \brief Everything a project file stores.
///
struct ProjectConfig
{
    /// \brief Server description.
    ServerSettings server;
    /// \brief Endpoints; enabled ones share one TCP port.
    QList<EndpointInfo> endpoints;
    /// \brief Nodes added under the Objects folder.
    QList<NodeDefinition> nodes;
    /// \brief NodeSet2 files loaded before the project nodes are built.
    QStringList nodeSetFiles;
};
