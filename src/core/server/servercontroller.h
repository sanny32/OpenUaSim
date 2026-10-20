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
    /// \brief Replaces the served project and rebuilds its nodes. Models the new project does not import are
    ///        unloaded by recreating the server, which restarts a running one; otherwise it keeps running.
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
    /// \brief Removes nodes with their children and rebuilds the address space once.
    /// \param nodeIds Nodes to remove; ids below an already removed node are skipped.
    /// \return False when the project has none of the nodes.
    ///
    bool removeNodes(const QStringList &nodeIds);

    ///
    /// \brief Copies a node with its children into a folder or object and rebuilds the address space.
    /// \param nodeId Node to copy.
    /// \param parentId Folder or object receiving the copy; empty for the top level.
    /// \param error Receives the reason on failure; may be null.
    /// \return Node id of the copy, or empty when it was refused. A taken name gets a "_copy" suffix,
    ///         and the copy and its children get node ids from their browse paths.
    ///
    QString copyNode(const QString &nodeId, const QString &parentId, QString *error = nullptr);

    ///
    /// \brief Applies edited variable properties.
    /// \param variable Node id with the new display name, icon, data type, access, configured quality and
    ///        simulation; a valid initial value, typed text or a value, replaces the stored one.
    /// \param error Receives the reason on failure; may be null.
    /// \return False when the change was refused, e.g. for text that is no value of the data type. A new
    ///         name, data type, array length or alarm rebuilds the address space; other changes apply to the live variable,
    ///         and a new initial value becomes its current value.
    ///
    bool updateVariable(const VariableInfo &variable, QString *error = nullptr);

    ///
    /// \brief Gives a folder, object or variable a new browse name and rebuilds the address space.
    /// \param nodeId Node to rename; its node id is kept.
    /// \param browseName New name.
    /// \param error Receives the reason on failure; may be null.
    /// \return False when the node is unknown or the name is empty or used by a sibling.
    ///
    bool renameNode(const QString &nodeId, const QString &browseName, QString *error = nullptr);

    ///
    /// \brief Replaces the server description; a running server applies it on the next restart.
    /// \param settings New application name, application URI and session limit.
    /// \param error Receives the reason on failure; may be null.
    /// \return False when the name or URI is empty or the session limit is outside 1..65535.
    ///
    bool setServerSettings(const ServerSettings &settings, QString *error = nullptr);

    ///
    /// \brief Replaces the user accounts; a running server applies them on the next restart.
    /// \param users Logins accepted by endpoints that take username/password tokens.
    /// \param error Receives the reason on failure; may be null.
    /// \return False when an account has no user name or two accounts share one.
    ///
    bool setUsers(const QList<UserAccount> &users, QString *error = nullptr);

    ///
    /// \brief Replaces the endpoints; a running server applies them on the next restart.
    /// \param endpoints New endpoint list.
    /// \param error Receives the reason on failure; may be null.
    /// \return False when enabled endpoints cannot be served together, e.g. on different ports.
    ///
    bool setEndpoints(const QList<EndpointInfo> &endpoints, QString *error = nullptr);

    ///
    /// \brief Loads a NodeSet2 model into the server and adds the file to the project.
    /// \param fileName NodeSet2 file.
    /// \param error Receives the reason on failure; may be null.
    /// \return False when the file cannot be loaded or was already imported; warnings go to the log.
    ///
    bool importNodeSet(const QString &fileName, QString *error = nullptr);

    ///
    /// \brief Unloads an imported NodeSet2 model and drops it from the project.
    /// \param fileName Model file as listed in the project.
    /// \param error Receives the reason on failure; may be null.
    /// \return False when the project has no such model. The server is recreated without the model;
    ///         a running server restarts, which disconnects its clients.
    ///
    bool removeNodeSet(const QString &fileName, QString *error = nullptr);

    ///
    /// \brief Returns the imported model each namespace came from.
    /// \return Model file keyed by namespace URI; namespaces 0 and 1 are not listed.
    ///
    QHash<QString, QString> modelNamespaces() const;

    ///
    /// \brief Writes the nodes of the application namespace, i.e. the project nodes, as a NodeSet2 file.
    /// \param fileName Target path.
    /// \param error Receives the reason on failure; may be null.
    /// \return True when the file was written; values that cannot be exported are logged.
    ///
    bool exportNodeSet(const QString &fileName, QString *error = nullptr);

    ///
    /// \brief Writes the nodes of one namespace as a NodeSet2 file.
    /// \param fileName Target path.
    /// \param namespaceUri URI of a served namespace other than the standard namespace 0.
    /// \param error Receives the reason on failure; may be null.
    /// \return True when the file was written; values that cannot be exported are logged.
    ///
    bool exportNamespace(const QString &fileName, const QString &namespaceUri, QString *error = nullptr);

    ///
    /// \brief Tells whether the running server still serves older server, endpoint or certificate settings.
    /// \return True after such a change on a running server until it restarts or stops.
    ///
    bool isRestartRequired() const;

    ///
    /// \brief Returns the PKI store, e.g. to point tests at a temporary directory.
    /// \return Store owned by the controller.
    ///
    CertificateStore &certificateStore();

    ///
    /// \brief Lists the certificates of the PKI store.
    /// \return Certificates of every folder.
    ///
    QList<CertificateInfo> certificates() const;

    ///
    /// \brief Adds a client or issuer certificate to the PKI store.
    /// \param fileName PEM or DER certificate.
    /// \param category Trusted, Rejected or Issuer folder.
    /// \param error Receives the reason on failure; may be null.
    /// \return True when the certificate was stored.
    ///
    bool importCertificate(const QString &fileName, CertificateInfo::Category category, QString *error = nullptr);

    ///
    /// \brief Installs the server certificate and private key used by secure endpoints.
    /// \param certificateFile PEM or DER certificate.
    /// \param keyFile PEM or DER private key.
    /// \param error Receives the reason on failure; may be null.
    /// \return True when both were stored.
    ///
    bool installServerCertificate(const QString &certificateFile, const QString &keyFile, QString *error = nullptr);

    ///
    /// \brief Creates a self-signed server certificate for the project and installs it, replacing an earlier one.
    /// \param error Receives the reason on failure; may be null.
    /// \return True when the certificate was created and stored.
    ///
    bool generateServerCertificate(QString *error = nullptr);

    ///
    /// \brief Creates a missing server certificate when generation is available and logs the result.
    ///
    void ensureServerCertificate();

    ///
    /// \brief Moves a stored certificate, e.g. to trust a rejected client.
    /// \param filePath Stored certificate file.
    /// \param category Target folder.
    /// \param error Receives the reason on failure; may be null.
    /// \return True when the certificate was moved.
    ///
    bool moveCertificate(const QString &filePath, CertificateInfo::Category category, QString *error = nullptr);

    ///
    /// \brief Deletes a stored certificate.
    /// \param filePath Stored certificate file.
    /// \param error Receives the reason on failure; may be null.
    /// \return True when the certificate was deleted.
    ///
    bool removeCertificate(const QString &filePath, QString *error = nullptr);

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
    /// \brief Announces that a node or endpoint edit changed the project.
    ///
    void projectEdited();

    ///
    /// \brief Announces new access or simulation settings of a variable whose node was kept.
    /// \param variable Updated description.
    ///
    void variableChanged(const VariableInfo &variable);

    ///
    /// \brief Announces a new endpoint list.
    ///
    void endpointsChanged();

    ///
    /// \brief Announces that the PKI store changed.
    ///
    void certificatesChanged();

    ///
    /// \brief Announces whether the server must restart to apply the endpoint settings.
    /// \param required New state.
    ///
    void restartRequiredChanged(bool required);

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

    ///
    /// \brief Announces that the server started with less than configured, e.g. without secure endpoints.
    /// \param reason Explanation for the user.
    ///
    void startWarning(const QString &reason);

private:
    void rebuildNodes();
    void setRestartRequired(bool required);
    bool loadNodeSet(const QString &fileName, QString *error);
    void loadProjectNodeSets();
    void reloadServer();
    bool finishCertificateChange(bool changed);
    void appendLog(const LogEntry &entry);
    void handleClientWrite(const QString &nodeId, const QVariant &value);
    void logMethodCall(const QString &methodId, const QString &result);
    void handleAlarmChanged(const QString &nodeId, const QString &state);
    void handleCertificateRejected(const QByteArray &certificate, const QString &reason);

    SimServer _server;
    AddressSpace _addressSpace;
    SimulationEngine _engine;
    ProjectConfig _project;
    QList<LogEntry> _log;
    bool _restartRequired = false;
    QStringList _loadedNodeSets;
    QHash<QString, QString> _modelNamespaces;
    CertificateStore _certificates;
};
