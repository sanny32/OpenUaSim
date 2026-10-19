// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file servercontroller.cpp
/// \brief Implements the facade that serves a project: server, address space and simulation.
///

#include <QFileInfo>

#include <QUaServer>

#include <algorithm>

#include "formatting.h"
#include "listenersettings.h"
#include "logmapper.h"
#include "pki/certificategenerator.h"
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
    connect(&_server, &SimServer::startWarning, this, &ServerController::startWarning);
    connect(&_engine, &SimulationEngine::valuesUpdated, this, &ServerController::valuesChanged);
    connect(&_addressSpace, &AddressSpace::valueWrittenByClient, this, &ServerController::handleClientWrite);
    connect(&_server, &SimServer::clientCertificateRejected, this, &ServerController::handleCertificateRejected);
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
    loadProjectNodeSets();
    rebuildNodes();
}

///
/// \brief Loads a NodeSet2 model into the server and adds the file to the project.
/// \param fileName NodeSet2 file.
/// \param error Receives the reason on failure; may be null.
/// \return False when the file cannot be loaded or was already imported; warnings go to the log.
///
bool ServerController::importNodeSet(const QString &fileName, QString *error)
{
    const QString path = QFileInfo(fileName).absoluteFilePath();
    if (_project.nodeSetFiles.contains(path)) {
        if (error)
            *error = tr("'%1' is already imported").arg(path);
        return false;
    }
    if (!loadNodeSet(path, error))
        return false;
    _project.nodeSetFiles.append(path);
    rebuildNodes();
    emit projectEdited();
    return true;
}

///
/// \brief Writes the nodes of the application namespace, i.e. the project nodes, as a NodeSet2 file.
/// \param fileName Target path.
/// \param error Receives the reason on failure; may be null.
/// \return True when the file was written; values that cannot be exported are logged.
///
bool ServerController::exportNodeSet(const QString &fileName, QString *error)
{
    const QString applicationNamespace = _server.uaServer()->namespaces().value(1);
    const QUaNodeSetExportResult result = _server.uaServer()->exportNodeSet(fileName, { applicationNamespace });
    for (const QString &warning : result.warnings)
        appendLog(LogMapper::entry(LogEntry::Level::Warning, LogEntry::Category::Server,
                                   tr("Model export: %1").arg(warning)));
    if (!result.isOk() && error)
        *error = result.errorString;
    return result.isOk();
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
    if (!nodeId.isEmpty()) {
        rebuildNodes();
        emit projectEdited();
    }
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
    emit projectEdited();
    return true;
}

///
/// \brief Applies edited variable properties.
/// \param variable Node id with the new display name, icon, data type, access and simulation.
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
    edited.icon = variable.icon;
    edited.dataType = variable.dataType;
    edited.access = variable.access;
    edited.simulation = variable.simulation;
    if (!ProjectNodes::updateVariable(_project.nodes, edited, error))
        return false;

    emit projectEdited();
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
    emit projectEdited();
    return true;
}

///
/// \brief Tells whether the running server still serves older endpoint or certificate settings.
/// \return True after such a change on a running server until it restarts or stops.
///
bool ServerController::isRestartRequired() const
{
    return _restartRequired;
}

///
/// \brief Returns the PKI store, e.g. to point tests at a temporary directory.
/// \return Store owned by the controller.
///
CertificateStore &ServerController::certificateStore()
{
    return _certificates;
}

///
/// \brief Lists the certificates of the PKI store.
/// \return Certificates of every folder.
///
QList<CertificateInfo> ServerController::certificates() const
{
    return _certificates.certificates();
}

///
/// \brief Adds a client or issuer certificate to the PKI store.
/// \param fileName PEM or DER certificate.
/// \param category Trusted, Rejected or Issuer folder.
/// \param error Receives the reason on failure; may be null.
/// \return True when the certificate was stored.
///
bool ServerController::importCertificate(const QString &fileName, CertificateInfo::Category category, QString *error)
{
    return finishCertificateChange(!_certificates.importCertificate(fileName, category, error).isEmpty());
}

///
/// \brief Installs the server certificate and private key used by secure endpoints.
/// \param certificateFile PEM or DER certificate.
/// \param keyFile PEM or DER private key.
/// \param error Receives the reason on failure; may be null.
/// \return True when both were stored.
///
bool ServerController::installServerCertificate(const QString &certificateFile, const QString &keyFile,
                                                QString *error)
{
    return finishCertificateChange(_certificates.installServerCertificate(certificateFile, keyFile, error));
}

///
/// \brief Creates a self-signed server certificate for the project and installs it, replacing an earlier one.
/// \param error Receives the reason on failure; may be null.
/// \return True when the certificate was created and stored.
///
bool ServerController::generateServerCertificate(QString *error)
{
    QStringList hostnames;
    for (const EndpointInfo &endpoint : std::as_const(_project.endpoints))
        hostnames.append(endpoint.hostname);
    const GeneratedCertificate generated =
        CertificateGenerator::generate(_project.server.applicationUri, hostnames, error);
    return finishCertificateChange(!generated.certificate.isEmpty()
                                   && _certificates.storeServerCertificate(generated.certificate,
                                                                           generated.privateKey, error));
}

///
/// \brief Moves a stored certificate, e.g. to trust a rejected client.
/// \param filePath Stored certificate file.
/// \param category Target folder.
/// \param error Receives the reason on failure; may be null.
/// \return True when the certificate was moved.
///
bool ServerController::moveCertificate(const QString &filePath, CertificateInfo::Category category, QString *error)
{
    return finishCertificateChange(!_certificates.move(filePath, category, error).isEmpty());
}

///
/// \brief Deletes a stored certificate.
/// \param filePath Stored certificate file.
/// \param error Receives the reason on failure; may be null.
/// \return True when the certificate was deleted.
///
bool ServerController::removeCertificate(const QString &filePath, QString *error)
{
    return finishCertificateChange(_certificates.remove(filePath, error));
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
    result.certificates = _certificates.certificates();
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
    ensureServerCertificate();
    _server.setPkiMaterial(_certificates.material());
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
/// \brief Loads a NodeSet2 file into the server once and logs its outcome.
/// \param fileName Absolute path.
/// \param error Receives the reason on failure; may be null.
/// \return True when the file is loaded, now or before.
///
bool ServerController::loadNodeSet(const QString &fileName, QString *error)
{
    if (_loadedNodeSets.contains(fileName))
        return true;
    const QUaNodeSetResult result = _server.uaServer()->loadNodeSet(fileName);
    if (!result.isOk()) {
        appendLog(LogMapper::entry(LogEntry::Level::Error, LogEntry::Category::Server,
                                   tr("Model '%1' not loaded: %2").arg(fileName, result.errorString)));
        if (error)
            *error = result.errorString;
        return false;
    }
    for (const QString &warning : result.warnings)
        appendLog(LogMapper::entry(LogEntry::Level::Warning, LogEntry::Category::Server,
                                   tr("Model '%1': %2").arg(fileName, warning)));
    appendLog(LogMapper::entry(LogEntry::Level::Info, LogEntry::Category::Server,
                               tr("Model '%1' loaded with %2 nodes").arg(fileName).arg(result.addedNodes.size())));
    _loadedNodeSets.append(fileName);
    return true;
}

///
/// \brief Loads the project's NodeSet2 files; models of earlier projects stay until the application restarts.
///
void ServerController::loadProjectNodeSets()
{
    for (const QString &fileName : std::as_const(_project.nodeSetFiles))
        loadNodeSet(fileName, nullptr);
    for (const QString &loaded : std::as_const(_loadedNodeSets)) {
        if (!_project.nodeSetFiles.contains(loaded))
            appendLog(LogMapper::entry(LogEntry::Level::Warning, LogEntry::Category::Server,
                                       tr("Model '%1' stays loaded until OpenUaSim restarts").arg(loaded)));
    }
}

///
/// \brief Creates the server certificate when a secure endpoint is enabled and none is installed.
///
void ServerController::ensureServerCertificate()
{
    if (!CertificateGenerator::isAvailable() || _certificates.hasServerCertificate()
        || !ListenerSettingsBuilder::usesSecurity(_project.endpoints))
        return;
    QString error;
    if (generateServerCertificate(&error))
        appendLog(LogMapper::entry(LogEntry::Level::Info, LogEntry::Category::Security,
                                   tr("Created a self-signed server certificate for the secure endpoints")));
    else
        appendLog(LogMapper::entry(LogEntry::Level::Error, LogEntry::Category::Security, error));
}

///
/// \brief Announces a successful PKI change, which a running server applies on restart.
/// \param changed Whether the store changed.
/// \return The value of changed, so callers can return it directly.
///
bool ServerController::finishCertificateChange(bool changed)
{
    if (!changed)
        return false;
    if (isRunning())
        setRestartRequired(true);
    emit certificatesChanged();
    return true;
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

///
/// \brief Keeps a rejected client certificate in the rejected folder, where it can be reviewed and trusted.
/// \param certificate Rejected DER certificate.
/// \param reason Status code name of the rejection.
///
void ServerController::handleCertificateRejected(const QByteArray &certificate, const QString &reason)
{
    QString error;
    const QString stored = _certificates.addCertificate(certificate, CertificateInfo::Category::Rejected, &error);
    if (stored.isEmpty()) {
        appendLog(LogMapper::entry(LogEntry::Level::Error, LogEntry::Category::Security,
                                   tr("A rejected client certificate could not be kept: %1").arg(error)));
        return;
    }
    appendLog(LogMapper::entry(LogEntry::Level::Warning, LogEntry::Category::Security,
                               tr("Client certificate '%1' rejected (%2); it was moved to the rejected folder")
                                   .arg(QFileInfo(stored).completeBaseName(), reason)));
    emit certificatesChanged();
}
