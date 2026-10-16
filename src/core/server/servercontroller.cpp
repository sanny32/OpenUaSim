// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file servercontroller.cpp
/// \brief Implements the facade that serves a project: server, address space and simulation.
///

#include <algorithm>

#include "formatting.h"
#include "listenersettings.h"
#include "logmapper.h"
#include "project/projectnodes.h"
#include "servercontroller.h"
#include "snapshotbuilder.h"

///
/// \brief Creates a controller serving an empty project.
/// \param parent Owning QObject.
///
ServerController::ServerController(QObject *parent)
    : QObject(parent)
    , _addressSpace(_server.uaServer())
    , _engine(&_addressSpace)
{
    connect(&_server, &SimServer::runningChanged, this, &ServerController::runningChanged);
    connect(&_server, &SimServer::logAppended, this, &ServerController::appendLog);
    connect(&_server, &SimServer::sessionsChanged, this, &ServerController::sessionsChanged);
    connect(&_server, &SimServer::startFailed, this, &ServerController::startFailed);
    connect(&_engine, &SimulationEngine::valuesUpdated, this, &ServerController::valuesChanged);
    connect(&_addressSpace, &AddressSpace::valueWrittenByClient, this, &ServerController::handleClientWrite);
}

///
/// \brief Stops the server.
///
ServerController::~ServerController()
{
    _engine.stop();
    _server.stop();
}

///
/// \brief Replaces the served project and rebuilds its nodes; a running server keeps running.
/// \param project Project to serve; nodes without node ids get ids from their browse paths.
///
void ServerController::setProject(const ProjectConfig &project)
{
    _project = project;
    ProjectNodes::assignMissingNodeIds(_project.nodes);
    rebuildNodes();
}

///
/// \brief Adds a node and rebuilds the address space.
/// \param parentId Folder or object id; empty for the top level.
/// \param node Node to add; an empty node id is assigned from the browse path.
/// \param error Receives the reason on failure; may be null.
/// \return Node id of the added node, or empty when it was refused.
///
QString ServerController::addNode(const QString &parentId, const NodeDefinition &node, QString *error)
{
    const QString nodeId = ProjectNodes::insert(_project.nodes, parentId, node, error);
    if (!nodeId.isEmpty())
        rebuildNodes();
    return nodeId;
}

///
/// \brief Removes a node with its children and rebuilds the address space.
/// \param nodeId Node to remove.
/// \return False when the project has no such node.
///
bool ServerController::removeNode(const QString &nodeId)
{
    if (!ProjectNodes::remove(_project.nodes, nodeId))
        return false;
    rebuildNodes();
    return true;
}

///
/// \brief Applies edited variable properties.
/// \param variable Node id with the new display name, data type, access and simulation.
/// \param error Receives the reason on failure; may be null.
/// \return False when the change was refused. A new name or data type rebuilds the address
///         space; access and simulation changes apply to the live variable.
///
bool ServerController::updateVariable(const VariableInfo &variable, QString *error)
{
    const NodeDefinition *existing = ProjectNodes::find(_project.nodes, variable.nodeId);
    if (!existing) {
        if (error)
            *error = tr("'%1' is not a variable").arg(variable.nodeId);
        return false;
    }
    const bool structural = existing->browseName != variable.displayName || existing->dataType != variable.dataType;

    NodeDefinition edited = *existing;
    edited.browseName = variable.displayName;
    edited.dataType = variable.dataType;
    edited.access = variable.access;
    edited.simulation = variable.simulation;
    if (!ProjectNodes::updateVariable(_project.nodes, edited, error))
        return false;

    if (structural) {
        rebuildNodes();
        return true;
    }
    _addressSpace.setAccess(edited.nodeId, edited.access);
    _engine.updateVariable({ edited.nodeId, edited.dataType, edited.simulation });
    emit variableChanged(SnapshotBuilder::variableInfo(*ProjectNodes::find(_project.nodes, edited.nodeId),
                                                       _addressSpace));
    return true;
}

///
/// \brief Replaces the endpoints; a running server applies them on the next restart.
/// \param endpoints New endpoint list.
/// \param error Receives the reason on failure; may be null.
/// \return False when enabled endpoints cannot be served together, e.g. on different ports.
///
bool ServerController::setEndpoints(const QList<EndpointInfo> &endpoints, QString *error)
{
    const bool anyEnabled = std::any_of(endpoints.cbegin(), endpoints.cend(),
                                        [](const EndpointInfo &endpoint) { return endpoint.enabled; });
    const ListenerSettings listener = ListenerSettingsBuilder::build(endpoints);
    if (anyEnabled && !listener.ok()) {
        if (error)
            *error = listener.error;
        return false;
    }
    _project.endpoints = endpoints;
    if (isRunning())
        setRestartRequired(true);
    emit endpointsChanged();
    return true;
}

///
/// \brief Tells whether the running server still serves older endpoint settings.
/// \return True after setEndpoints() on a running server until it restarts or stops.
///
bool ServerController::isRestartRequired() const
{
    return _restartRequired;
}

///
/// \brief Returns the served project.
/// \return Project with every node id assigned.
///
const ProjectConfig &ServerController::project() const
{
    return _project;
}

///
/// \brief Tells whether the server is listening.
/// \return True while running.
///
bool ServerController::isRunning() const
{
    return _server.isRunning();
}

///
/// \brief Builds everything the main window shows.
/// \param now Current time, for the uptime.
/// \return Snapshot including the kept log.
///
ServerSnapshot ServerController::snapshot(const QDateTime &now) const
{
    ServerSnapshot result = SnapshotBuilder::build(_project, _server, _addressSpace, now);
    result.log = _log;
    return result;
}

///
/// \brief Builds the server summary.
/// \param now Current time, for the uptime.
/// \return Summary for the status page and the status bar.
///
ServerStatus ServerController::status(const QDateTime &now) const
{
    return SnapshotBuilder::status(_project, _server, _addressSpace, now);
}

///
/// \brief Returns the active and recently closed sessions.
/// \return Sessions, active ones first.
///
QList<SessionInfo> ServerController::sessions() const
{
    return _server.sessions();
}

///
/// \brief Returns the kept log.
/// \return Entries, oldest first.
///
const QList<LogEntry> &ServerController::log() const
{
    return _log;
}

///
/// \brief Returns the server, e.g. for tests that connect clients.
/// \return Server owned by the controller.
///
SimServer &ServerController::server()
{
    return _server;
}

///
/// \brief Returns the built nodes.
/// \return Address space owned by the controller.
///
AddressSpace &ServerController::addressSpace()
{
    return _addressSpace;
}

///
/// \brief Starts the server and the simulation.
/// \return True when the server runs; otherwise startFailed() tells why.
///
bool ServerController::start()
{
    if (!_server.start(_project))
        return false;
    _engine.start();
    setRestartRequired(false);
    return true;
}

///
/// \brief Stops the server and the simulation.
///
void ServerController::stop()
{
    _engine.stop();
    _server.stop();
    setRestartRequired(false);
}

///
/// \brief Stops and starts the server, applying changed endpoint settings.
/// \return True when the server runs again.
///
bool ServerController::restart()
{
    stop();
    return start();
}

///
/// \brief Builds the project nodes again, drives their simulation and announces the new structure.
///
void ServerController::rebuildNodes()
{
    const QStringList problems = _addressSpace.build(_project.nodes);
    for (const QString &problem : problems)
        appendLog(LogMapper::entry(LogEntry::Level::Warning, LogEntry::Category::Server,
                                   tr("Node skipped: %1").arg(problem)));
    _engine.setVariables(SimulationEngine::variablesOf(_project.nodes));
    emit projectChanged();
}

///
/// \brief Records whether a restart is pending and announces changes.
/// \param required New state.
///
void ServerController::setRestartRequired(bool required)
{
    if (_restartRequired == required)
        return;
    _restartRequired = required;
    emit restartRequiredChanged(required);
}

///
/// \brief Keeps a log entry and announces it.
/// \param entry Entry to append.
///
void ServerController::appendLog(const LogEntry &entry)
{
    _log.append(entry);
    if (_log.size() > kLogLimit)
        _log.remove(0, _log.size() - kLogLimit);
    emit logAppended(entry);
}

///
/// \brief Logs and announces a value written by a client.
/// \param nodeId Variable node id.
/// \param value Written value.
///
void ServerController::handleClientWrite(const QString &nodeId, const QVariant &value)
{
    appendLog(LogMapper::entry(LogEntry::Level::Info, LogEntry::Category::DataChanges,
                               tr("Client wrote %1 to %2").arg(Formatting::value(value), nodeId)));
    emit valuesChanged({ { nodeId, value } });
}
