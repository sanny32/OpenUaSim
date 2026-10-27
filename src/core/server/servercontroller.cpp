// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file servercontroller.cpp
/// \brief Implements the facade that serves a project: server, address space and simulation.
///

#include <QFileInfo>

#include <QUaProperty>
#include <QUaServer>

#include <algorithm>

#include "declarednodes.h"
#include "formatting.h"
#include "importednodes.h"
#include "listenersettings.h"
#include "logmapper.h"
#include "modelvariables.h"
#include "project/alarmrules.h"
#include "project/projectnodes.h"
#include "project/serversettingsrules.h"
#include "project/useraccountrules.h"
#include "servercontroller.h"
#include "simulation/valuecast.h"
#include "snapshotbuilder.h"
#include "typecatalog.h"
#include "variableproperties.h"

///
/// \brief Creates a controller serving an empty project.
/// \param parent Owning QObject.
///
ServerController::ServerController(QObject *parent)
    : QObject(parent)
    , _addressSpace(_server.uaServer())
    , _engine(&_addressSpace)
    , _nodeSets(_server)
    , _pki(_server)
{
    connect(&_server, &SimServer::runningChanged, this, &ServerController::runningChanged);
    connect(&_server, &SimServer::logAppended, this, &ServerController::appendLog);
    connect(&_server, &SimServer::sessionsChanged, this, &ServerController::sessionsChanged);
    connect(&_server, &SimServer::startFailed, this, &ServerController::startFailed);
    connect(&_server, &SimServer::startWarning, this, &ServerController::startWarning);
    connect(&_engine, &SimulationEngine::valuesUpdated, this, &ServerController::valuesChanged);
    connect(&_addressSpace, &AddressSpace::valueWrittenByClient, this, &ServerController::handleClientWrite);
    connect(&_addressSpace, &AddressSpace::qualityChanged, this, &ServerController::variableQualityChanged);
    connect(&_addressSpace, &AddressSpace::valuesSetByMethod, this, &ServerController::valuesChanged);
    connect(&_addressSpace, &AddressSpace::methodCalled, this, &ServerController::logMethodCall);
    connect(&_addressSpace, &AddressSpace::alarmChanged, this, &ServerController::handleAlarmChanged);
    connect(&_nodeSets, &NodeSetLoader::logAppended, this, &ServerController::appendLog);
    connect(&_pki, &PkiService::certificatesChanged, this, &ServerController::certificatesChanged);
    connect(&_pki, &PkiService::restartRequired, this, [this] { setRestartRequired(true); });
    connect(&_pki, &PkiService::logAppended, this, &ServerController::appendLog);
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
    const QStringList loaded = _nodeSets.loaded();
    const bool unusedModels = std::any_of(loaded.cbegin(), loaded.cend(),
                                          [this](const QString &file) { return !_project.nodeSetFiles.contains(file); });
    if (unusedModels) {
        reloadServer();
        return;
    }
    _nodeSets.loadAll(_project.nodeSetFiles);
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
    const QHash<QString, QString> modelFiles = _nodeSets.modelNamespaces();
    _project.modelVariables.removeIf([&modelFiles, &fileName](const ModelVariable &variable) {
        return modelFiles.value(variable.namespaceUri) == fileName;
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
    for (QString &file : _project.nodeSetFiles)
        file = movedFiles.value(file, file);
    _nodeSets.renameFiles(movedFiles);
}

///
/// \brief Returns the imported model each namespace came from.
/// \return Model file keyed by namespace URI; namespaces 0 and 1 are not listed.
///
QHash<QString, QString> ServerController::modelNamespaces() const
{
    return _nodeSets.modelNamespaces();
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
    if (!_nodeSets.load(path, error))
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
    return _nodeSets.exportApplicationNamespace(fileName, error);
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
    return _nodeSets.exportNamespace(fileName, namespaceUri, error);
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
    const QString problem = TypeCatalog::variableProblem(*_server.uaServer(), node);
    if (!problem.isEmpty()) {
        if (error)
            *error = problem;
        return {};
    }
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
/// \param variable Node id with the new display name, icon, data type, access, configured quality, historizing,
///        simulation and edited property values; a valid initial value, typed text or a value, replaces the stored
///        one. An imported model variable keeps the name, data type and array length of its model; a property of a
///        variable takes only its value text, as initial value.
/// \param error Receives the reason on failure; may be null.
/// \return False when the change was refused, e.g. for text that is no value of the data type. A new name, data
///         type, array length, alarm or property value rebuilds the address space; other changes apply to the live
///         variable, and a new initial value becomes its current value.
///
bool ServerController::updateVariable(const VariableInfo &variable, QString *error)
{
    if (variable.property)
        return updateProperty(variable, error);
    const NodeDefinition *existing = ProjectNodes::find(_project.nodes, variable.nodeId);
    if (!existing && ModelVariables::find(*_server.uaServer(), variable.nodeId))
        return updateModelVariable(variable, error);
    if (!existing)
        return updateDeclaredVariable(variable, error);
    NodeDefinition edited = *existing;
    edited.browseName = variable.displayName;
    edited.dataType = variable.dataType;
    edited.arrayLength = variable.arrayLength;
    const QString problem = TypeCatalog::variableProblem(*_server.uaServer(), edited);
    if (!problem.isEmpty()) {
        if (error)
            *error = problem;
        return false;
    }
    if (!applyVariableEdits(variable, edited, error))
        return false;
    const bool structural = existing->browseName != edited.browseName || existing->dataType != edited.dataType
        || existing->arrayLength != edited.arrayLength || existing->alarm != edited.alarm
        || existing->properties != edited.properties;
    if (!ProjectNodes::updateVariable(_project.nodes, edited, error))
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
/// \param variable Server node id with the new icon, access, configured quality, historizing, simulation, alarm
///        and, when valid, initial value; the model keeps defining the name, data type and array length.
/// \param error Receives the reason on failure; may be null.
/// \return False for a node that is no model variable the simulation can drive, or for refused settings. A new
///         alarm rebuilds the address space; other changes apply to the live variable.
///
bool ServerController::updateModelVariable(const VariableInfo &variable, QString *error)
{
    const QUaBaseDataVariable *served = ModelVariables::find(*_server.uaServer(), variable.nodeId);
    if (!served || !ModelVariables::isEditable(*_server.uaServer(), *served)) {
        if (error)
            *error = tr("'%1' is not a variable").arg(variable.nodeId);
        return false;
    }
    ModelVariable model = ModelVariables::describe(*_server.uaServer(), *served);
    const qsizetype index =
        ModelVariables::indexOf(_project.modelVariables, model.namespaceUri, model.variable.nodeId);
    const NodeDefinition stored = index >= 0 ? _project.modelVariables.at(index).variable : NodeDefinition();
    model.variable.initialValue = stored.initialValue;
    model.variable.properties = stored.properties;
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
    if (stored.alarm != model.variable.alarm || stored.properties != model.variable.properties) {
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
/// \brief Applies the value typed for a property of a variable as an edited property value of that variable.
/// \param property Property node id with the value text as initial value; an invalid one leaves the value as is.
/// \param error Receives the reason on failure; may be null.
/// \return False for a node that is no property of a listed variable, or for text that is no value of its data type.
///
bool ServerController::updateProperty(const VariableInfo &property, QString *error)
{
    const auto *served = _server.uaServer()->nodeById<QUaProperty>(QUaNodeId(property.nodeId));
    const auto *owner = served ? qobject_cast<const QUaBaseDataVariable *>(served->parent()) : nullptr;
    VariableInfo variable = owner ? listedVariable(owner->nodeId()) : VariableInfo();
    if (variable.nodeId.isEmpty()) {
        if (error)
            *error = tr("'%1' is not a property of a variable").arg(property.nodeId);
        return false;
    }
    if (!property.initialValue.isValid())
        return true;
    for (PropertyInfo &info : variable.properties) {
        if (info.browseName == served->browseName().name()) {
            info.value = property.initialValue.toString();
            info.edited = true;
        }
    }
    variable.initialValue = QVariant();
    return updateVariable(variable, error);
}

///
/// \brief Finds the description of a variable as the views list it.
/// \param nodeId Server node id of the variable.
/// \return Description from the snapshot, or an empty one when no folder, object or variable lists the variable.
///
VariableInfo ServerController::listedVariable(const QString &nodeId) const
{
    const QHash<QString, QList<VariableInfo>> variables = snapshot().variables;
    for (const QList<VariableInfo> &list : variables) {
        for (const VariableInfo &info : list) {
            if (info.nodeId == nodeId)
                return info;
        }
    }
    return {};
}

///
/// \brief Keeps the first settings of a variable that the type of a project instance declares and rebuilds the
///        address space, which applies them.
/// \param variable Server node id with the new icon, access, configured quality, historizing, simulation, alarm
///        and, when valid, initial value; the type keeps defining the name, data type and array length.
/// \param error Receives the reason on failure; may be null.
/// \return False for a node that no project instance declares, a variable the simulation cannot drive, or
///         refused settings.
///
bool ServerController::updateDeclaredVariable(const VariableInfo &variable, QString *error)
{
    QUaServer &server = *_server.uaServer();
    const auto *served = server.nodeById<QUaBaseDataVariable>(QUaNodeId(variable.nodeId));
    if (!served || !ModelVariables::isEditable(server, *served)) {
        if (error)
            *error = tr("'%1' is not a variable").arg(variable.nodeId);
        return false;
    }
    NodeDefinition settings = ModelVariables::describe(server, *served).variable;
    settings.nodeId = variable.nodeId;
    if (!applyVariableEdits(variable, settings, error))
        return false;
    if (const QString problem = AlarmRules::problem(settings.alarm, settings.dataType, settings.arrayLength);
        !problem.isEmpty()) {
        if (error)
            *error = problem;
        return false;
    }
    if (!DeclaredNodes::keep(_project.nodes, *served, settings)) {
        if (error)
            *error = tr("'%1' is not a variable").arg(variable.nodeId);
        return false;
    }
    emit projectEdited();
    rebuildNodes();
    return true;
}

///
/// \brief Copies the edited icon, access, quality, historizing, simulation, alarm, initial value and property values
///        of a variable.
/// \param variable Edited properties; a valid initial value, typed text or a value, replaces the stored one, and an
///        edited property value replaces the stored one or, when empty, drops it.
/// \param node Definition receiving the edits; its data type parses the initial value.
/// \param error Receives the reason on failure; may be null.
/// \return False for an initial value or a property value that is no value of its data type.
///
bool ServerController::applyVariableEdits(const VariableInfo &variable, NodeDefinition &node, QString *error)
{
    node.icon = variable.icon;
    node.alarm = variable.alarm;
    node.access = variable.access;
    node.quality = variable.configuredQuality;
    node.historizing = variable.historizing;
    node.simulation = variable.simulation;
    if (!applyPropertyEdits(variable.properties, node.properties, error))
        return false;
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
/// \brief Copies the edited property values of a variable in their canonical text form.
/// \param edited Properties as shown, with the edited ones marked.
/// \param values Stored value texts keyed by browse name; an edited empty text drops the value.
/// \param error Receives the reason on failure; may be null.
/// \return False for text that is no value of the data type of its property; values is then unchanged.
///
bool ServerController::applyPropertyEdits(const QList<PropertyInfo> &edited, QMap<QString, QString> &values,
                                          QString *error)
{
    QMap<QString, QString> result = values;
    for (const PropertyInfo &property : edited) {
        if (!property.edited)
            continue;
        if (property.value.trimmed().isEmpty()) {
            result.remove(property.browseName);
            continue;
        }
        const QVariant value = VariableProperties::fromText(property.value, property.dataType);
        if (!value.isValid()) {
            if (error)
                *error = tr("'%1' is not a %2 value for '%3'").arg(property.value, property.dataType,
                                                                   property.browseName);
            return false;
        }
        result.insert(property.browseName, VariableProperties::toText(value, property.dataType));
    }
    values = result;
    return true;
}

///
/// \brief Applies the access, quality, historizing, icon and simulation of a served variable without rebuilding the
///        address space.
/// \param node Stored definition with the server node id.
/// \param valueEdited Whether the initial value changed and becomes the current value.
///
void ServerController::applyLiveSettings(const NodeDefinition &node, bool valueEdited)
{
    _addressSpace.setAccess(node.nodeId, node.access);
    _addressSpace.setQuality(node.nodeId, node.quality);
    _addressSpace.setHistorizing(node.nodeId, node.historizing);
    _addressSpace.setIcon(node.nodeId, node.icon);
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
    const QUaBaseDataVariable *model = node.declared
        ? _server.uaServer()->nodeById<QUaBaseDataVariable>(QUaNodeId(node.nodeId))
        : ModelVariables::find(*_server.uaServer(), node.nodeId);
    if (model)
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
/// \param settings New application name, application URI, session limit and history limits.
/// \param error Receives the reason on failure; may be null.
/// \return False when ServerSettingsRules::problem() refuses the settings.
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
    if (uriChanged && _pki.store().hasServerCertificate())
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
/// \brief Returns the PKI operations: certificate store changes and client certificate reviews.
/// \return Service owned by the controller.
///
PkiService &ServerController::pki()
{
    return _pki;
}

///
/// \brief Creates a self-signed server certificate for the project and installs it, replacing an earlier one.
/// \param error Receives the reason on failure; may be null.
/// \return True when the certificate was created and stored.
///
bool ServerController::generateServerCertificate(QString *error)
{
    return _pki.generateServerCertificate(_project.server.applicationUri, certificateHostnames(), error);
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
    const QHash<QString, QString> modelFiles = _nodeSets.modelNamespaces();
    for (NamespaceInfo &ns : result.namespaces)
        ns.modelFile = modelFiles.value(ns.uri);
    result.log = _log;
    result.certificates = _pki.certificates();
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
    _server.setPkiMaterial(_pki.store().material());
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
/// \brief Recreates the server with the project's models and nodes only; a running server restarts.
///
void ServerController::reloadServer()
{
    const bool wasRunning = isRunning();
    stop();
    _addressSpace.clear();
    _server.reset();
    _addressSpace.setServer(_server.uaServer());
    _nodeSets.clear();
    _nodeSets.loadAll(_project.nodeSetFiles);
    rebuildNodes();
    if (wasRunning)
        start();
}

///
/// \brief Creates a missing server certificate for the project when generation is available and logs the result.
///
void ServerController::ensureServerCertificate()
{
    _pki.ensureServerCertificate(_project.server.applicationUri, certificateHostnames());
}

///
/// \brief Lists the host names of the endpoints, which the server certificate names.
/// \return Host name of every endpoint.
///
QStringList ServerController::certificateHostnames() const
{
    QStringList hostnames;
    for (const EndpointInfo &endpoint : std::as_const(_project.endpoints))
        hostnames.append(endpoint.hostname);
    return hostnames;
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
