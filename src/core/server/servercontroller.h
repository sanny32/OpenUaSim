// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file servercontroller.h
/// \brief Declares the facade that serves a project: server, address space and simulation.
///

#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QVariant>

#include "addressspace.h"
#include "data/projectconfig.h"
#include "data/serversnapshot.h"
#include "simserver.h"
#include "simulation/simulationengine.h"

///
/// \brief Serves one project: builds its nodes, runs the server and the simulation, and keeps the log.
///
class ServerController : public QObject
{
    Q_OBJECT

public:
    /// \brief Number of log entries kept; older ones are dropped first.
    static constexpr int kLogLimit = 5000;

    ///
    /// \brief Creates a controller serving an empty project.
    /// \param parent Owning QObject.
    ///
    explicit ServerController(QObject *parent = nullptr);

    ///
    /// \brief Stops the server.
    ///
    ~ServerController() override;

    ///
    /// \brief Replaces the served project and rebuilds its nodes; a running server keeps running.
    /// \param project Project to serve; nodes without node ids get ids from their browse paths.
    ///
    void setProject(const ProjectConfig &project);

    ///
    /// \brief Adds a node and rebuilds the address space.
    /// \param parentId Folder or object id; empty for the top level.
    /// \param node Node to add; an empty node id is assigned from the browse path.
    /// \param error Receives the reason on failure; may be null.
    /// \return Node id of the added node, or empty when it was refused.
    ///
    QString addNode(const QString &parentId, const NodeDefinition &node, QString *error = nullptr);

    ///
    /// \brief Removes a node with its children and rebuilds the address space.
    /// \param nodeId Node to remove.
    /// \return False when the project has no such node.
    ///
    bool removeNode(const QString &nodeId);

    ///
    /// \brief Applies edited variable properties.
    /// \param variable Node id with the new display name, data type, access and simulation.
    /// \param error Receives the reason on failure; may be null.
    /// \return False when the change was refused. A new name or data type rebuilds the address
    ///         space; access and simulation changes apply to the live variable.
    ///
    bool updateVariable(const VariableInfo &variable, QString *error = nullptr);

    ///
    /// \brief Returns the served project.
    /// \return Project with every node id assigned.
    ///
    const ProjectConfig &project() const;

    ///
    /// \brief Tells whether the server is listening.
    /// \return True while running.
    ///
    bool isRunning() const;

    ///
    /// \brief Builds everything the main window shows.
    /// \param now Current time, for the uptime.
    /// \return Snapshot including the kept log.
    ///
    ServerSnapshot snapshot(const QDateTime &now = QDateTime::currentDateTime()) const;

    ///
    /// \brief Builds the server summary.
    /// \param now Current time, for the uptime.
    /// \return Summary for the status page and the status bar.
    ///
    ServerStatus status(const QDateTime &now = QDateTime::currentDateTime()) const;

    ///
    /// \brief Returns the active and recently closed sessions.
    /// \return Sessions, active ones first.
    ///
    QList<SessionInfo> sessions() const;

    ///
    /// \brief Returns the kept log.
    /// \return Entries, oldest first.
    ///
    const QList<LogEntry> &log() const;

    ///
    /// \brief Returns the server, e.g. for tests that connect clients.
    /// \return Server owned by the controller.
    ///
    SimServer &server();

    ///
    /// \brief Returns the built nodes.
    /// \return Address space owned by the controller.
    ///
    AddressSpace &addressSpace();

public slots:
    ///
    /// \brief Starts the server and the simulation.
    /// \return True when the server runs; otherwise startFailed() tells why.
    ///
    bool start();

    ///
    /// \brief Stops the server and the simulation.
    ///
    void stop();

    ///
    /// \brief Stops and starts the server, applying changed endpoint settings.
    /// \return True when the server runs again.
    ///
    bool restart();

signals:
    ///
    /// \brief Announces that the server started or stopped.
    /// \param running New state.
    ///
    void runningChanged(bool running);

    ///
    /// \brief Announces a new project; views reload the whole snapshot.
    ///
    void projectChanged();

    ///
    /// \brief Announces new access or simulation settings of a variable whose node was kept.
    /// \param variable Updated description.
    ///
    void variableChanged(const VariableInfo &variable);

    ///
    /// \brief Announces variable values changed by the simulation or by clients.
    /// \param values New values keyed by node id.
    ///
    void valuesChanged(const QHash<QString, QVariant> &values);

    ///
    /// \brief Announces a new log entry.
    /// \param entry Appended entry.
    ///
    void logAppended(const LogEntry &entry);

    ///
    /// \brief Announces that a session opened or closed.
    ///
    void sessionsChanged();

    ///
    /// \brief Announces that the server could not start.
    /// \param reason Explanation for the user.
    ///
    void startFailed(const QString &reason);

private:
    void rebuildNodes();
    void appendLog(const LogEntry &entry);
    void handleClientWrite(const QString &nodeId, const QVariant &value);

    SimServer _server;
    AddressSpace _addressSpace;
    SimulationEngine _engine;
    ProjectConfig _project;
    QList<LogEntry> _log;
};
