// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file defaultproject.cpp
/// \brief Implements the project a new document starts from.
///

#include "defaultproject.h"
#include "server/listenersettings.h"

namespace {

///
/// \brief Builds a node identifier in the application namespace.
/// \param path Dotted string identifier.
/// \return Node id such as "ns=1;s=Motor_01.Speed".
///
QString nodeId(const QString &path)
{
    return QStringLiteral("ns=1;s=%1").arg(path);
}

///
/// \brief Builds a simulated Double variable.
/// \param device Owning device, used to derive the node id.
/// \param name Browse name.
/// \param icon Icon resource name.
/// \param function Waveform.
/// \param intervalMs Update interval.
/// \param amplitude Peak deviation.
/// \param offset Centre value.
/// \return Read/write historizing variable starting at the offset.
///
NodeDefinition analog(const QString &device, const QString &name, const QString &icon,
                      SimulationFunction function, int intervalMs, double amplitude, double offset)
{
    NodeDefinition node;
    node.kind = NodeDefinition::Kind::Variable;
    node.browseName = name;
    node.nodeId = nodeId(device + QLatin1Char('.') + name);
    node.icon = icon;
    node.dataType = QStringLiteral("Double");
    node.initialValue = offset;
    node.simulation.function = function;
    node.simulation.intervalMs = intervalMs;
    node.simulation.amplitude = amplitude;
    node.simulation.offset = offset;
    node.historizing = true;
    return node;
}

///
/// \brief Builds a writable Boolean variable that is not simulated.
/// \param device Owning device, used to derive the node id.
/// \param name Browse name.
/// \param icon Icon resource name.
/// \param value Initial value.
/// \return Read/write historizing variable.
///
NodeDefinition flag(const QString &device, const QString &name, const QString &icon, bool value)
{
    NodeDefinition node;
    node.kind = NodeDefinition::Kind::Variable;
    node.browseName = name;
    node.nodeId = nodeId(device + QLatin1Char('.') + name);
    node.icon = icon;
    node.dataType = QStringLiteral("Boolean");
    node.initialValue = value;
    node.historizing = true;
    return node;
}

///
/// \brief Builds one endpoint for every security policy and mode pair OPC UA allows.
/// \return Endpoints on localhost and the default port, starting with the unsecured one.
///
QList<EndpointInfo> allEndpoints()
{
    QList<EndpointInfo> endpoints;
    for (const auto &pair : ListenerSettingsBuilder::allowedSecurityPairs()) {
        EndpointInfo endpoint;
        endpoint.protocol = QStringLiteral("opc.tcp");
        endpoint.hostname = QStringLiteral("localhost");
        endpoint.securityPolicy = pair.first;
        endpoint.securityMode = pair.second;
        endpoints.append(endpoint);
    }
    return endpoints;
}

///
/// \brief Builds a folder or object with children.
/// \param kind Folder or Object.
/// \param name Browse name, also the node id.
/// \param children Child nodes.
/// \return Node.
///
NodeDefinition container(NodeDefinition::Kind kind, const QString &name,
                         const QList<NodeDefinition> &children)
{
    NodeDefinition node;
    node.kind = kind;
    node.browseName = name;
    node.nodeId = nodeId(name);
    node.icon = QStringLiteral("folder");
    node.children = children;
    return node;
}

}

namespace DefaultProject {

///
/// \brief Builds the sample factory-line project.
/// \return Project with an anonymous endpoint on port 48400 for every security policy and mode
///         pair, and a FactoryLine_01 folder holding a motor, a conveyor, a pump and a tank with
///         simulated variables.
///
ProjectConfig create()
{
    const QString motor = QStringLiteral("Motor_01");
    const QString conveyor = QStringLiteral("Conveyor_01");
    const QString pump = QStringLiteral("Pump_01");
    const QString tank = QStringLiteral("Tank_01");
    const QString signal = QStringLiteral("signal");
    const QString gauge = QStringLiteral("gauge");
    const QString thermometer = QStringLiteral("thermometer");
    const QString running = QStringLiteral("status-good");
    const auto object = NodeDefinition::Kind::Object;

    ProjectConfig project;
    project.endpoints = allEndpoints();

    project.nodes.append(container(NodeDefinition::Kind::Folder, QStringLiteral("FactoryLine_01"), {
        container(object, motor, {
            analog(motor, QStringLiteral("Speed"), signal, SimulationFunction::Sine, 100, 500.0, 1000.0),
            analog(motor, QStringLiteral("Temperature"), thermometer, SimulationFunction::Ramp, 1000, 10.0, 40.0),
            flag(motor, QStringLiteral("Running"), running, true),
        }),
        container(object, conveyor, {
            analog(conveyor, QStringLiteral("Speed"), signal, SimulationFunction::Square, 2000, 0.4, 1.0),
            analog(conveyor, QStringLiteral("Load"), gauge, SimulationFunction::Random, 1000, 20.0, 60.0),
            flag(conveyor, QStringLiteral("Running"), running, true),
        }),
        container(object, pump, {
            analog(pump, QStringLiteral("Flow"), signal, SimulationFunction::Sine, 250, 3.0, 12.0),
            analog(pump, QStringLiteral("Pressure"), gauge, SimulationFunction::Random, 500, 0.5, 4.0),
            flag(pump, QStringLiteral("Running"), running, true),
        }),
        container(object, tank, {
            analog(tank, QStringLiteral("Level"), gauge, SimulationFunction::Triangle, 1000, 25.0, 50.0),
            analog(tank, QStringLiteral("Temperature"), thermometer, SimulationFunction::Random, 5000, 1.5, 21.0),
            flag(tank, QStringLiteral("InletValve"), QStringLiteral("status-inactive"), false),
        }),
    }));
    return project;
}

}
