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
#include "importednodes.h"
#include "listenersettings.h"
#include "logmapper.h"
#include "modelvariables.h"
#include "pki/certificategenerator.h"
#include "project/alarmrules.h"
#include "project/projectnodes.h"
#include "project/serversettingsrules.h"
#include "project/useraccountrules.h"
#include "servercontroller.h"
#include "simulation/valuecast.h"
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
    connect(&_addressSpace, &AddressSpace::valuesSetByMethod, this, &ServerController::valuesChanged);
    connect(&_addressSpace, &AddressSpace::methodCalled, this, &ServerController::logMethodCall);
    connect(&_addressSpace, &AddressSpace::alarmChanged, this, &ServerController::handleAlarmChanged);
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
/// \brief Replaces the served project and rebuilds its nodes. Models the new project does not import are
///        unloaded by recreating the server, which restarts a running one; otherwise it keeps running.
/// \param project Project to serve; nodes without node ids get ids from their browse paths.
///
void ServerController::setProject(const ProjectConfig &project)
{
    _project = project;
    ProjectNodes::assignMissingNodeIds(_project.nodes);
    const bool unusedModels = std::any_of(_loadedNodeSets.cbegin(), _loadedNodeSets.cend(),
                                          [this](const QString &file) { return !_project.nodeSetFiles.contains(file); });
    if (unusedModels) {
        reloadServer();
        return;
    }
    loadProjectNodeSets();
    rebuildNodes();
}

///
/// \brief Unloads an imported NodeSet2 model and drops it from the project.
/// \param fileName Model file as listed in the project.
/// \param error Receives the reason on failure; may be null.
/// \return False when the project has no such model. The server is recreated without the model;
///         a running server restarts, which disconnects its clients.
///
bool ServerController::removeNodeSet(const QString &fileName, QString *error)
{
    if (!_project.nodeSetFiles.removeOne(fileName)) {
        if (error)
            *error = tr("'%1' is not imported").arg(fileName);
        return false;
    }
    _project.modelVariables.removeIf([this, &fileName](const ModelVariable &variable) {
        return _modelNamespaces.value(variable.namespaceUri) == fileName;
    });
    reloadServer();
    emit projectEdited();
    return true;
}

///
/// \brief Points the project at moved model files without reloading them, e.g. after they were copied next to
///        the project file.
/// \param movedFiles New absolute path keyed by the former one; paths the project does not import are ignored.
///
void ServerController::renameNodeSetFiles(const QHash<QString, QString> &movedFiles)
{
    for (QStringList *files : { &_project.nodeSetFiles, &_loadedNodeSets }) {
        for (QString &file : *files)
            file = movedFiles.value(file, file);
    }
    for (QString &file : _modelNamespaces)
        file = movedFiles.value(file, file);
}

///
/// \brief Returns the imported model each namespace came from.
/// \return Model file keyed by namespace URI; namespaces 0 and 1 are not listed.
///
QHash<QString, QString> ServerController::modelNamespaces() const
{
    return _modelNamespaces;
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
    return exportNamespace(fileName, _server.uaServer()->namespaces().value(1), error);
}

///
/// \brief Writes the nodes of one namespace as a NodeSet2 file.
/// \param fileName Target path.
/// \param namespaceUri URI of a served namespace other than the standard namespace 0.
/// \param error Receives the reason on failure; may be null.
/// \return True when the file was written; values that cannot be exported are logged.
///
bool ServerController::exportNamespace(const QString &fileName, const QString &namespaceUri, QString *error)
{
    if (namespaceUri.isEmpty() || namespaceUri == _server.uaServer()->namespaces().value(0)) {
        if (error)
            *error = tr("The standard OPC UA namespace cannot be exported");
        return false;
    }
    const QUaNodeSetExportResult result = _server.uaServer()->exportNodeSet(fileName, { namespaceUri });
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
    return removeNodes({ nodeId });
}

///
/// \brief Removes nodes with their children and rebuilds the address space once.
/// \param nodeIds Nodes to remove; ids below an already removed node are skipped.
/// \return False when the project has none of the nodes.
///
bool ServerController::removeNodes(const QStringList &nodeIds)
{
    bool removed = false;
    for (const QString &nodeId : nodeIds)
        removed |= ProjectNodes::remove(_project.nodes, nodeId);
    if (!removed)
        return false;
    rebuildNodes();
    emit projectEdited();
    return true;
}

///
/// \brief Copies a node with its children into a folder or object and rebuilds the address space.
/// \param nodeId Node to copy.
/// \param parentId Folder or object receiving the copy; empty for the top level.
/// \param error Receives the reason on failure; may be null.
/// \return Node id of the copy, or empty when it was refused. A taken name gets a "_copy" suffix,
///         and the copy and its children get node ids from their browse paths.
///
QString ServerController::copyNode(const QString &nodeId, const QString &parentId, QString *error)
{
    const NodeDefinition *source = ProjectNodes::find(_project.nodes, nodeId);
    if (!source) {
        if (error)
            *error = tr("There is no node '%1'").arg(nodeId);
        return {};
    }
    const NodeDefinition copy =
        ProjectNodes::detachedCopy(*source, ProjectNodes::childNames(_project.nodes, parentId));
    return addNode(parentId, copy, error);
}

///
/// \brief Applies edited variable properties.
/// \param variable Node id with the new display name, icon, data type, access, configured quality and
///        simulation; a valid initial value, typed text or a value, replaces the stored one.
/// \param error Receives the reason on failure; may be null.
/// \return False when the change was refused, e.g. for text that is no value of the data type. A new
///         name, data type, array length or alarm rebuilds the address space; other changes apply to the live variable,
///         and a new initial value becomes its current value.
///
bool ServerController::updateVariable(const VariableInfo &variable, QString *error)
{
    const NodeDefinition *existing = ProjectNodes::find(_project.nodes, variable.nodeId);
    if (!existing)
        return updateModelVariable(variable, error);
    const bool structural = existing->browseName != variable.displayName || existing->dataType != variable.dataType
        || existing->arrayLength != variable.arrayLength || existing->alarm != variable.alarm;

    NodeDefinition edited = *existing;
    edited.browseName = variable.displayName;
    edited.dataType = variable.dataType;
    edited.arrayLength = variable.arrayLength;
    if (!applyVariableEdits(variable, edited, error) || !ProjectNodes::updateVariable(_project.nodes, edited, error))
        return false;

    emit projectEdited();
    if (structural) {
        rebuildNodes();
        return true;
    }
    applyLiveSettings(*ProjectNodes::find(_project.nodes, edited.nodeId), variable.initialValue.isValid());
    return true;
}

///
/// \brief Applies edited properties of an imported model variable and keeps them in the project.
/// \param variable Server node id with the new icon, access, configured quality, simulation, alarm and, when
///        valid, initial value; the model keeps defining the name, data type and array length.
/// \param error Receives the reason on failure; may be null.
/// \return False for a node that is no model variable the simulation can drive, or for refused settings. A new
///         alarm rebuilds the address space; other changes apply to the live variable.
///
bool ServerController::updateModelVariable(const VariableInfo &variable, QString *error)
{
    const QUaBaseDataVariable *served = ModelVariables::find(*_server.uaServer(), variable.nodeId);
    if (!served || !ModelVariables::isEditable(*served)) {
        if (error)
            *error = tr("'%1' is not a variable").arg(variable.nodeId);
        return false;
    }
    ModelVariable model = ModelVariables::describe(*_server.uaServer(), *served);
    const qsizetype index =
        ModelVariables::indexOf(_project.modelVariables, model.namespaceUri, model.variable.nodeId);
    const AlarmSettings storedAlarm = index >= 0 ? _project.modelVariables.at(index).variable.alarm : AlarmSettings();
    if (index >= 0)
        model.variable.initialValue = _project.modelVariables.at(index).variable.initialValue;
    if (!applyVariableEdits(variable, model.variable, error))
        return false;
    if (const QString problem =
            AlarmRules::problem(model.variable.alarm, model.variable.dataType, model.variable.arrayLength);
        !problem.isEmpty()) {
        if (error)
            *error = problem;
        return false;
    }
    if (index >= 0)
        _project.modelVariables[index] = model;
    else
        _project.modelVariables.append(model);

    emit projectEdited();
    if (storedAlarm != model.variable.alarm) {
        rebuildNodes();
        return true;
    }
    NodeDefinition settings = model.variable;
    settings.nodeId = variable.nodeId;
    const auto entry = std::find_if(_modelSettings.begin(), _modelSettings.end(),
                                    [&settings](const NodeDefinition &node) { return node.nodeId == settings.nodeId; });
    if (entry != _modelSettings.end())
        *entry = settings;
    else
        _modelSettings.append(settings);
    _addressSpace.attach({ settings });
    applyLiveSettings(settings, variable.initialValue.isValid());
    return true;
}

///
/// \brief Copies the edited icon, access, quality, simulation, alarm and initial value of a variable.
/// \param variable Edited properties; a valid initial value, typed text or a value, replaces the stored one.
/// \param node Definition receiving the edits; its data type parses the initial value.
/// \param error Receives the reason on failure; may be null.
/// \return False for an initial value that is no value of the data type.
///
bool ServerController::applyVariableEdits(const VariableInfo &variable, NodeDefinition &node, QString *error)
{
    node.icon = variable.icon;
    node.alarm = variable.alarm;
    node.access = variable.access;
    node.quality = variable.configuredQuality;
    node.simulation = variable.simulation;
    if (!variable.initialValue.isValid())
        return true;
    node.initialValue = variable.initialValue.typeId() == QMetaType::QString
        ? ValueCast::fromText(variable.initialValue.toString(), node.dataType)
        : variable.initialValue;
    if (node.initialValue.isValid())
        return true;
    if (error)
        *error = tr("'%1' is not a %2 value").arg(variable.initialValue.toString(), node.dataType);
    return false;
}

///
/// \brief Applies the access, quality and simulation of a served variable without rebuilding the address space.
/// \param node Stored definition with the server node id.
/// \param valueEdited Whether the initial value changed and becomes the current value.
///
void ServerController::applyLiveSettings(const NodeDefinition &node, bool valueEdited)
{
    _addressSpace.setAccess(node.nodeId, node.access);
    _addressSpace.setQuality(node.nodeId, node.quality);
    if (valueEdited)
        _addressSpace.setValue(node.nodeId, node.initialValue);
    _engine.updateVariable({ node.nodeId, node.dataType, node.simulation, node.arrayLength });
    emit variableChanged(variableInfo(node));
}

///
/// \brief Describes a project or model variable for the views.
/// \param node Stored definition with the server node id.
/// \return Description with the live value and alarm state.
///
VariableInfo ServerController::variableInfo(const NodeDefinition &node) const
{
    if (const QUaBaseDataVariable *model = ModelVariables::find(*_server.uaServer(), node.nodeId))
        return ImportedNodes::variableInfo(*_server.uaServer(), *model, &node, _addressSpace);
    return SnapshotBuilder::variableInfo(node, _addressSpace);
}

///
/// \brief Gives a folder, object or variable a new browse name and rebuilds the address space.
/// \param nodeId Node to rename; its node id is kept.
/// \param browseName New name.
/// \param error Receives the reason on failure; may be null.
/// \return False when the node is unknown or the name is empty or used by a sibling.
///
bool ServerController::renameNode(const QString &nodeId, const QString &browseName, QString *error)
{
    if (!ProjectNodes::rename(_project.nodes, nodeId, browseName, error))
        return false;
    emit projectEdited();
    rebuildNodes();
    return true;
}

///
/// \brief Replaces the server description; a running server applies it on the next restart.
/// \param settings New application name, application URI and session limit.
/// \param error Receives the reason on failure; may be null.
/// \return False when the name or URI is empty or the session limit is outside 1..65535.
///
bool ServerController::setServerSettings(const ServerSettings &settings, QString *error)
{
    const QString problem = ServerSettingsRules::problem(settings);
    if (!problem.isEmpty()) {
        if (error)
            *error = problem;
        return false;
    }

    const bool uriChanged = settings.applicationUri != _project.server.applicationUri;
    _project.server = settings;
    if (uriChanged && _certificates.hasServerCertificate())
        appendLog(LogMapper::entry(LogEntry::Level::Warning, LogEntry::Category::Security,
                                   tr("The server certificate names the former application URI; "
                                      "create a new one on the Certificates page")));
    if (isRunning())
        setRestartRequired(true);
    emit projectEdited();
    return true;
}

///
/// \brief Replaces the user accounts; a running server applies them on the next restart.
/// \param users Logins accepted by endpoints that take username/password tokens.
/// \param error Receives the reason on failure; may be null.
/// \return False when an account has no user name or two accounts share one.
///
bool ServerController::setUsers(const QList<UserAccount> &users, QString *error)
{
    const QString problem = UserAccountRules::problem(users);
    if (!problem.isEmpty()) {
        if (error)
            *error = problem;
        return false;
    }
    _project.users = users;
    if (isRunning())
        setRestartRequired(true);
    emit projectEdited();
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
/// \brief Tells whether the running server still serves older server, endpoint or certificate settings.
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
    for (NamespaceInfo &ns : result.namespaces)
        ns.modelFile = _modelNamespaces.value(ns.uri);
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
    if (ListenerSettingsBuilder::usesSecurity(_project.endpoints))
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
    QStringList modelProblems;
    _modelSettings = ModelVariables::resolve(_project.modelVariables, *_server.uaServer(), &modelProblems);
    modelProblems += _addressSpace.attach(_modelSettings);
    for (const QString &problem : std::as_const(modelProblems))
        appendLog(LogMapper::entry(LogEntry::Level::Warning, LogEntry::Category::Server,
                                   tr("Model variable settings skipped: %1").arg(problem)));
    _engine.setVariables(SimulationEngine::variablesOf(_project.nodes)
                         + SimulationEngine::variablesOf(_modelSettings));
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
    const QStringList namespacesBefore = _server.uaServer()->namespaces();
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
    for (const QString &uri : _server.uaServer()->namespaces()) {
        if (!namespacesBefore.contains(uri))
            _modelNamespaces.insert(uri, fileName);
    }
    return true;
}

///
/// \brief Loads the project's NodeSet2 files that are not loaded yet.
///
void ServerController::loadProjectNodeSets()
{
    for (const QString &fileName : std::as_const(_project.nodeSetFiles))
        loadNodeSet(fileName, nullptr);
}

///
/// \brief Recreates the server with the project's models and nodes only; a running server restarts.
///
void ServerController::reloadServer()
{
    const bool wasRunning = isRunning();
    stop();
    _addressSpace.clear();
    _server.reset();
    _addressSpace.setServer(_server.uaServer());
    _loadedNodeSets.clear();
    _modelNamespaces.clear();
    loadProjectNodeSets();
    rebuildNodes();
    if (wasRunning)
        start();
}

///
/// \brief Creates a missing server certificate when generation is available and logs the result.
///
void ServerController::ensureServerCertificate()
{
    if (!CertificateGenerator::isAvailable() || _certificates.hasServerCertificate())
        return;
    QString error;
    if (generateServerCertificate(&error))
        appendLog(LogMapper::entry(LogEntry::Level::Info, LogEntry::Category::Security,
                                   tr("Created a self-signed server certificate")));
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
/// \brief Logs an alarm becoming active or inactive and announces the variable's new alarm state.
/// \param nodeId Variable node id.
/// \param state Active state, e.g. "High"; empty when the alarm became inactive.
///
void ServerController::handleAlarmChanged(const QString &nodeId, const QString &state)
{
    const NodeDefinition *node = ProjectNodes::find(_project.nodes, nodeId);
    if (!node)
        node = ProjectNodes::find(_modelSettings, nodeId);
    if (!node)
        return;
    appendLog(state.isEmpty()
                  ? LogMapper::entry(LogEntry::Level::Info, LogEntry::Category::DataChanges,
                                     tr("Alarm of %1 returned to normal").arg(node->browseName))
                  : LogMapper::entry(LogEntry::Level::Warning, LogEntry::Category::DataChanges,
                                     tr("Alarm of %1 is active: %2").arg(node->browseName, state)));
    emit variableChanged(variableInfo(*node));
}

///
/// \brief Logs a method call of a client.
/// \param methodId Node id of the called method.
/// \param result Result text returned to the client.
///
void ServerController::logMethodCall(const QString &methodId, const QString &result)
{
    appendLog(LogMapper::entry(LogEntry::Level::Info, LogEntry::Category::DataChanges,
                               tr("Client called %1: %2").arg(methodId, result)));
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
