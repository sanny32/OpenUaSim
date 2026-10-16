// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file servercontroller.cpp
/// \brief Implements the facade that serves a project: server, address space and simulation.
///

#include "formatting.h"
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
    const QStringList problems = _addressSpace.build(_project.nodes);
    for (const QString &problem : problems)
        appendLog(LogMapper::entry(LogEntry::Level::Warning, LogEntry::Category::Server,
                                   tr("Node skipped: %1").arg(problem)));
    _engine.setVariables(SimulationEngine::variablesOf(_project.nodes));
    emit projectChanged();
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
    return true;
}

///
/// \brief Stops the server and the simulation.
///
void ServerController::stop()
{
    _engine.stop();
    _server.stop();
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
