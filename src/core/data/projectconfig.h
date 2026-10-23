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
    enum class Kind { Folder, Object, Variable, Method };

    ///
    /// \brief What a method does with the variables next to it.
    ///
    enum class MethodAction {
        Reset,    ///< Sets every sibling variable to its initial value.
        SetValue  ///< Sets the sibling variable named by the first argument to the value text in the second.
    };

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
    /// \brief Number of elements of an array variable; 0 for a scalar. Variables only.
    int arrayLength = 0;
    /// \brief Quality reported with every value, e.g. Bad to test client handling; variables only.
    VariableInfo::Quality quality = VariableInfo::Quality::Good;
    /// \brief Simulation driving the value; variables only.
    SimulationSettings simulation;
    /// \brief Alarm raised from the value; variables only.
    AlarmSettings alarm;
    /// \brief Whether the server records the values for clients to read with HistoryRead; variables only.
    bool historizing = false;
    /// \brief Action of a method; methods only.
    MethodAction method = MethodAction::Reset;
    /// \brief Child nodes; folders and objects only.
    QList<NodeDefinition> children;
};

///
/// \brief Settings a project keeps for a variable of an imported model, whose structure the model defines.
///
struct ModelVariable
{
    /// \brief URI of the model namespace holding the variable.
    QString namespaceUri;
    /// \brief Icon, access, quality, initial value, simulation and alarm of the variable. Its node id is the
    ///        identifier within the namespace, e.g. "i=1003"; an invalid initial value keeps the model value.
    NodeDefinition variable;
};

///
/// \brief Login that endpoints accepting username/password tokens check clients against.
///
struct UserAccount
{
    /// \brief User name.
    QString username;
    /// \brief Password.
    QString password;
};

/// \brief Largest array a variable may hold, keeping the simulation and the tables responsive.
constexpr int kMaxArrayLength = 1000;

/// \brief Largest session limit the server takes; QUaServer stores it as a 16-bit count.
constexpr int kMaxSessionLimit = 65535;

/// \brief Largest number of values the history keeps per variable.
constexpr int kMaxHistoryValues = 1000000;

/// \brief Longest time the history keeps a value, one week.
constexpr int kMaxHistoryAgeMinutes = 7 * 24 * 60;

///
/// \brief How much of the value history of historizing variables the server keeps in memory.
///
struct HistoryLimits
{
    /// \brief Values kept per variable, the oldest dropped first; bounds the memory whatever the update rate.
    int maxValues = 1000;
    /// \brief Minutes a value is kept, counted back from the newest value of its variable; 0 keeps values until
    ///        maxValues drops them.
    int maxAgeMinutes = 0;
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
    /// \brief Value history kept for historizing variables.
    HistoryLimits history;
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
    /// \brief Logins accepted by the endpoints that take username/password tokens.
    QList<UserAccount> users;
    /// \brief Nodes added under the Objects folder.
    QList<NodeDefinition> nodes;
    /// \brief Absolute paths of the NodeSet2 files loaded before the project nodes are built.
    QStringList nodeSetFiles;
    /// \brief Settings of imported model variables that differ from their model.
    QList<ModelVariable> modelVariables;
};
