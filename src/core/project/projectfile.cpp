// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectfile.cpp
/// \brief Implements reading and writing of OpenUaSim project files.
///

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>

#include <tuple>

#include "alarmrules.h"
#include "projectfile.h"
#include "simulation/signalgenerator.h"
#include "simulation/valuecast.h"

namespace {

const QStringList kAccessLevels = { QStringLiteral("Read"), QStringLiteral("Write"),
                                    QStringLiteral("Read/Write") };

///
/// \brief Translates a message of the project file reader.
/// \param text Source text.
/// \return Translated text.
///
QString tr(const char *text)
{
    return QCoreApplication::translate("ProjectFile", text);
}

///
/// \brief Returns the stored name of a node kind.
/// \param kind Node kind.
/// \return "folder", "object", "variable" or "method".
///
QString kindKey(NodeDefinition::Kind kind)
{
    switch (kind) {
    case NodeDefinition::Kind::Folder:   return QStringLiteral("folder");
    case NodeDefinition::Kind::Object:   return QStringLiteral("object");
    case NodeDefinition::Kind::Variable: return QStringLiteral("variable");
    case NodeDefinition::Kind::Method:   return QStringLiteral("method");
    }
    return {};
}

///
/// \brief Returns the stored name of a method action.
/// \param action Action to name.
/// \return "reset" or "setValue".
///
QString methodKey(NodeDefinition::MethodAction action)
{
    switch (action) {
    case NodeDefinition::MethodAction::Reset:    return QStringLiteral("reset");
    case NodeDefinition::MethodAction::SetValue: return QStringLiteral("setValue");
    }
    return {};
}

///
/// \brief Parses the stored name of a node kind.
/// \param key Stored name.
/// \param kind Receives the kind.
/// \return False for an unknown name.
///
bool kindFromKey(const QString &key, NodeDefinition::Kind &kind)
{
    for (NodeDefinition::Kind candidate : { NodeDefinition::Kind::Folder, NodeDefinition::Kind::Object,
                                            NodeDefinition::Kind::Variable, NodeDefinition::Kind::Method }) {
        if (kindKey(candidate) == key) {
            kind = candidate;
            return true;
        }
    }
    return false;
}

///
/// \brief Returns the stored key of a value quality.
/// \param quality Quality to name.
/// \return Snake-case name of the status code, e.g. "good" or "bad_sensor_failure".
///
QString qualityKey(VariableInfo::Quality quality)
{
    using Q = VariableInfo::Quality;
    switch (quality) {
    case Q::Good:                                    return QStringLiteral("good");
    case Q::GoodLocalOverride:                       return QStringLiteral("good_local_override");
    case Q::Uncertain:                               return QStringLiteral("uncertain");
    case Q::UncertainNoCommunicationLastUsableValue: return QStringLiteral("uncertain_no_communication_last_usable_value");
    case Q::UncertainLastUsableValue:                return QStringLiteral("uncertain_last_usable_value");
    case Q::UncertainSubstituteValue:                return QStringLiteral("uncertain_substitute_value");
    case Q::UncertainInitialValue:                   return QStringLiteral("uncertain_initial_value");
    case Q::UncertainSensorNotAccurate:              return QStringLiteral("uncertain_sensor_not_accurate");
    case Q::UncertainEngineeringUnitsExceeded:       return QStringLiteral("uncertain_engineering_units_exceeded");
    case Q::UncertainSubNormal:                      return QStringLiteral("uncertain_sub_normal");
    case Q::Bad:                                     return QStringLiteral("bad");
    case Q::BadConfigurationError:                   return QStringLiteral("bad_configuration_error");
    case Q::BadNotConnected:                         return QStringLiteral("bad_not_connected");
    case Q::BadDeviceFailure:                        return QStringLiteral("bad_device_failure");
    case Q::BadSensorFailure:                        return QStringLiteral("bad_sensor_failure");
    case Q::BadOutOfService:                         return QStringLiteral("bad_out_of_service");
    case Q::BadNoCommunication:                      return QStringLiteral("bad_no_communication");
    case Q::BadWaitingForInitialData:                return QStringLiteral("bad_waiting_for_initial_data");
    }
    return {};
}

///
/// \brief Parses a stored value quality.
/// \param key Stored key.
/// \param quality Receives the quality when the key is known.
/// \return False for an unknown key.
///
bool qualityFromKey(const QString &key, VariableInfo::Quality &quality)
{
    for (VariableInfo::Quality candidate : VariableInfo::qualities()) {
        if (qualityKey(candidate) == key) {
            quality = candidate;
            return true;
        }
    }
    return false;
}

///
/// \brief Converts a variable value to JSON.
/// \param value Value of a supported data type.
/// \return Boolean, string or number; DateTime values as ISO 8601 text.
///
QJsonValue valueToJson(const QVariant &value)
{
    switch (value.typeId()) {
    case QMetaType::Bool:      return value.toBool();
    case QMetaType::QString:   return value.toString();
    case QMetaType::QDateTime: return value.toDateTime().toString(Qt::ISODateWithMs);
    default:                   return value.isValid() ? QJsonValue(value.toDouble()) : QJsonValue();
    }
}

///
/// \brief Converts a stored value to a data type.
/// \param json Stored value; undefined yields the type default.
/// \param dataType Built-in data type name.
/// \return Value of the matching Qt type.
///
QVariant valueFromJson(const QJsonValue &json, const QString &dataType)
{
    if (json.isUndefined() || json.isNull())
        return ValueCast::defaultValue(dataType);
    if (dataType == QLatin1String("Boolean"))
        return json.toBool();
    if (dataType == QLatin1String("String"))
        return json.toString();
    if (dataType == QLatin1String("DateTime"))
        return QDateTime::fromString(json.toString(), Qt::ISODateWithMs);
    return ValueCast::fromDouble(json.toDouble(), dataType);
}

///
/// \brief Serializes simulation settings.
/// \param simulation Settings to write.
/// \return JSON object.
///
QJsonObject simulationToJson(const SimulationSettings &simulation)
{
    QJsonObject json {
        { QStringLiteral("function"), SignalGenerator::key(simulation.function) },
        { QStringLiteral("intervalMs"), simulation.intervalMs },
        { QStringLiteral("periodMs"), simulation.periodMs },
        { QStringLiteral("amplitude"), simulation.amplitude },
        { QStringLiteral("offset"), simulation.offset },
    };
    if (!simulation.values.isEmpty())
        json.insert(QStringLiteral("values"), QJsonArray::fromStringList(simulation.values));
    return json;
}

///
/// \brief Serializes alarm settings; limits are stored only when they apply.
/// \param alarm Settings to write.
/// \return JSON object.
///
QJsonObject alarmToJson(const AlarmSettings &alarm)
{
    QJsonObject json {
        { QStringLiteral("severity"), alarm.severity },
        { QStringLiteral("normalValue"), alarm.normalValue },
    };
    for (const auto &[key, enabled, limit] : { std::tuple { "highHigh", alarm.highHighEnabled, alarm.highHigh },
                                               std::tuple { "high", alarm.highEnabled, alarm.high },
                                               std::tuple { "low", alarm.lowEnabled, alarm.low },
                                               std::tuple { "lowLow", alarm.lowLowEnabled, alarm.lowLow } }) {
        if (enabled)
            json.insert(QLatin1String(key), limit);
    }
    return json;
}

///
/// \brief Parses alarm settings; a stored limit enables it.
/// \param json Stored settings.
/// \return Enabled alarm settings.
///
AlarmSettings alarmFromJson(const QJsonObject &json)
{
    AlarmSettings alarm;
    alarm.enabled = true;
    alarm.severity = json.value(QStringLiteral("severity")).toInt(alarm.severity);
    alarm.normalValue = json.value(QStringLiteral("normalValue")).toBool();
    alarm.highHighEnabled = json.contains(QStringLiteral("highHigh"));
    alarm.highHigh = json.value(QStringLiteral("highHigh")).toDouble();
    alarm.highEnabled = json.contains(QStringLiteral("high"));
    alarm.high = json.value(QStringLiteral("high")).toDouble();
    alarm.lowEnabled = json.contains(QStringLiteral("low"));
    alarm.low = json.value(QStringLiteral("low")).toDouble();
    alarm.lowLowEnabled = json.contains(QStringLiteral("lowLow"));
    alarm.lowLow = json.value(QStringLiteral("lowLow")).toDouble();
    return alarm;
}

///
/// \brief Serializes a node and its children.
/// \param node Node to write.
/// \return JSON object; variable fields are written for variables only.
///
QJsonObject nodeToJson(const NodeDefinition &node)
{
    QJsonObject json {
        { QStringLiteral("kind"), kindKey(node.kind) },
        { QStringLiteral("browseName"), node.browseName },
        { QStringLiteral("nodeId"), node.nodeId },
        { QStringLiteral("icon"), node.icon },
    };
    if (!node.typeDefinition.isEmpty() && node.kind != NodeDefinition::Kind::Method)
        json.insert(QStringLiteral("typeDefinition"), node.typeDefinition);
    if (node.declared)
        json.insert(QStringLiteral("declared"), true);
    if (node.kind == NodeDefinition::Kind::Variable) {
        json.insert(QStringLiteral("dataType"), node.dataType);
        json.insert(QStringLiteral("access"), node.access);
        if (node.initialValue.isValid())
            json.insert(QStringLiteral("initialValue"), valueToJson(node.initialValue));
        if (node.arrayLength > 0)
            json.insert(QStringLiteral("arrayLength"), node.arrayLength);
        json.insert(QStringLiteral("quality"), qualityKey(node.quality));
        json.insert(QStringLiteral("simulation"), simulationToJson(node.simulation));
        if (node.alarm.enabled)
            json.insert(QStringLiteral("alarm"), alarmToJson(node.alarm));
        if (node.historizing)
            json.insert(QStringLiteral("historizing"), true);
        if (!node.properties.isEmpty()) {
            QJsonObject properties;
            for (auto it = node.properties.cbegin(); it != node.properties.cend(); ++it)
                properties.insert(it.key(), it.value());
            json.insert(QStringLiteral("properties"), properties);
        }
    } else if (node.kind == NodeDefinition::Kind::Method) {
        json.insert(QStringLiteral("action"), methodKey(node.method));
    } else {
        QJsonArray children;
        for (const NodeDefinition &child : node.children)
            children.append(nodeToJson(child));
        json.insert(QStringLiteral("children"), children);
    }
    return json;
}

///
/// \brief Serializes the settings of a model variable.
/// \param variable Settings to write.
/// \return Variable node object with the namespace URI.
///
QJsonObject modelVariableToJson(const ModelVariable &variable)
{
    QJsonObject json = nodeToJson(variable.variable);
    json.insert(QStringLiteral("namespaceUri"), variable.namespaceUri);
    return json;
}

///
/// \brief Serializes an endpoint.
/// \param endpoint Endpoint to write.
/// \return JSON object.
///
QJsonObject endpointToJson(const EndpointInfo &endpoint)
{
    return {
        { QStringLiteral("protocol"), endpoint.protocol },
        { QStringLiteral("hostname"), endpoint.hostname },
        { QStringLiteral("port"), endpoint.port },
        { QStringLiteral("path"), endpoint.path },
        { QStringLiteral("securityPolicy"), endpoint.securityPolicy },
        { QStringLiteral("securityMode"), endpoint.securityMode },
        { QStringLiteral("enabled"), endpoint.enabled },
        { QStringLiteral("allowAnonymous"), endpoint.allowAnonymous },
        { QStringLiteral("usernamePassword"), endpoint.usernamePassword },
        // Key of the former "Require Client Certificate" option, kept so existing projects load unchanged.
        { QStringLiteral("requireClientCertificate"), endpoint.trustedClientsOnly },
    };
}

///
/// \brief Parses an endpoint; missing fields keep their defaults.
/// \param json Stored endpoint.
/// \return Endpoint.
///
EndpointInfo endpointFromJson(const QJsonObject &json)
{
    EndpointInfo endpoint;
    endpoint.protocol = json.value(QStringLiteral("protocol")).toString(QStringLiteral("opc.tcp"));
    endpoint.hostname = json.value(QStringLiteral("hostname")).toString(QStringLiteral("localhost"));
    endpoint.port = json.value(QStringLiteral("port")).toInt(endpoint.port);
    endpoint.path = json.value(QStringLiteral("path")).toString();
    endpoint.securityPolicy = json.value(QStringLiteral("securityPolicy")).toString(QStringLiteral("None"));
    endpoint.securityMode = json.value(QStringLiteral("securityMode")).toString(QStringLiteral("None"));
    endpoint.enabled = json.value(QStringLiteral("enabled")).toBool(endpoint.enabled);
    endpoint.allowAnonymous = json.value(QStringLiteral("allowAnonymous")).toBool(endpoint.allowAnonymous);
    endpoint.usernamePassword = json.value(QStringLiteral("usernamePassword")).toBool(endpoint.usernamePassword);
    endpoint.trustedClientsOnly =
        json.value(QStringLiteral("requireClientCertificate")).toBool(endpoint.trustedClientsOnly);
    return endpoint;
}

///
/// \brief Parses a user account.
/// \param json Stored account, or a format 1 endpoint holding its login.
/// \return Account; the user name is empty when none is stored.
///
UserAccount userFromJson(const QJsonObject &json)
{
    return { json.value(QStringLiteral("username")).toString(), json.value(QStringLiteral("password")).toString() };
}

///
/// \brief Adds a user account unless its name is empty or already listed.
/// \param users Accounts to extend.
/// \param user Account to add.
///
void addUser(QList<UserAccount> &users, const UserAccount &user)
{
    if (user.username.isEmpty())
        return;
    for (const UserAccount &existing : std::as_const(users)) {
        if (existing.username == user.username)
            return;
    }
    users.append(user);
}

///
/// \brief Parses the node tree of a project and reports the first invalid node.
///
class NodeReader
{
    Q_DECLARE_TR_FUNCTIONS(NodeReader)

public:
    bool read(const QJsonArray &array, const QString &path, QList<NodeDefinition> &nodes);
    bool readModelVariables(const QJsonArray &array, const QString &path, QList<ModelVariable> &variables);
    QString error() const { return _error; }

private:
    bool readNode(const QJsonObject &json, const QString &path, NodeDefinition &node);
    bool readVariable(const QJsonObject &json, const QString &path, NodeDefinition &node);
    bool fail(const QString &path, const QString &message);

    QSet<QString> _nodeIds;
    QString _error;
};

///
/// \brief Parses an array of nodes.
/// \param array Stored nodes.
/// \param path Location of the array for error messages, e.g. "nodes[0].children".
/// \param nodes Receives the parsed nodes.
/// \return False when a node is invalid; error() tells why.
///
bool NodeReader::read(const QJsonArray &array, const QString &path, QList<NodeDefinition> &nodes)
{
    for (qsizetype index = 0; index < array.size(); ++index) {
        NodeDefinition node;
        if (!readNode(array.at(index).toObject(), QStringLiteral("%1[%2]").arg(path).arg(index), node))
            return false;
        nodes.append(node);
    }
    return true;
}

///
/// \brief Parses the settings of model variables.
/// \param array Stored settings.
/// \param path Location of the array for error messages.
/// \param variables Receives the parsed settings.
/// \return False when settings are invalid, lack the namespace URI or repeat a variable; error() tells why.
///
bool NodeReader::readModelVariables(const QJsonArray &array, const QString &path, QList<ModelVariable> &variables)
{
    QSet<QString> keys;
    for (qsizetype index = 0; index < array.size(); ++index) {
        const QString itemPath = QStringLiteral("%1[%2]").arg(path).arg(index);
        const QJsonObject json = array.at(index).toObject();
        ModelVariable variable;
        variable.namespaceUri = json.value(QStringLiteral("namespaceUri")).toString();
        if (variable.namespaceUri.isEmpty())
            return fail(itemPath, tr("namespace URI is empty"));
        // Identifiers repeat across model namespaces, so only the URI and identifier together must be unique.
        _nodeIds.clear();
        if (!readNode(json, itemPath, variable.variable))
            return false;
        if (variable.variable.kind != NodeDefinition::Kind::Variable)
            return fail(itemPath, tr("model settings belong to a variable"));
        if (variable.variable.nodeId.isEmpty())
            return fail(itemPath, tr("node id is empty"));
        if (!json.contains(QStringLiteral("initialValue")))
            variable.variable.initialValue = QVariant();
        const QString key = variable.namespaceUri + QLatin1Char('\n') + variable.variable.nodeId;
        if (keys.contains(key))
            return fail(itemPath, tr("the variable has settings more than once"));
        keys.insert(key);
        variables.append(variable);
    }
    return true;
}

///
/// \brief Parses one node and its children.
/// \param json Stored node.
/// \param path Location of the node for error messages.
/// \param node Receives the parsed node.
/// \return False when the node is invalid.
///
bool NodeReader::readNode(const QJsonObject &json, const QString &path, NodeDefinition &node)
{
    const QString kind = json.value(QStringLiteral("kind")).toString();
    if (!kindFromKey(kind, node.kind))
        return fail(path, tr("unknown node kind '%1'").arg(kind));

    node.browseName = json.value(QStringLiteral("browseName")).toString();
    if (node.browseName.isEmpty())
        return fail(path, tr("browse name is empty"));

    node.nodeId = json.value(QStringLiteral("nodeId")).toString();
    if (!node.nodeId.isEmpty()) {
        if (_nodeIds.contains(node.nodeId))
            return fail(path, tr("node id '%1' is used more than once").arg(node.nodeId));
        _nodeIds.insert(node.nodeId);
    }
    node.icon = json.value(QStringLiteral("icon")).toString();
    if (node.kind != NodeDefinition::Kind::Method)
        node.typeDefinition = json.value(QStringLiteral("typeDefinition")).toString();
    node.declared = json.value(QStringLiteral("declared")).toBool(false);

    if (node.kind == NodeDefinition::Kind::Variable)
        return readVariable(json, path, node);
    if (node.kind == NodeDefinition::Kind::Method) {
        const QString action = json.value(QStringLiteral("action")).toString();
        for (auto candidate : { NodeDefinition::MethodAction::Reset, NodeDefinition::MethodAction::SetValue }) {
            if (methodKey(candidate) == action) {
                node.method = candidate;
                return true;
            }
        }
        return fail(path, tr("unknown method action '%1'").arg(action));
    }
    return read(json.value(QStringLiteral("children")).toArray(), path + QStringLiteral(".children"),
                node.children);
}

///
/// \brief Parses the variable fields of a node.
/// \param json Stored node.
/// \param path Location of the node for error messages.
/// \param node Variable receiving the fields.
/// \return False for an unsupported data type, access level, quality, simulation function or alarm.
///
bool NodeReader::readVariable(const QJsonObject &json, const QString &path, NodeDefinition &node)
{
    node.dataType = json.value(QStringLiteral("dataType")).toString(node.dataType);
    if (!ValueCast::isSupported(node.dataType))
        return fail(path, tr("unsupported data type '%1'").arg(node.dataType));

    node.access = json.value(QStringLiteral("access")).toString(node.access);
    if (!kAccessLevels.contains(node.access))
        return fail(path, tr("unknown access level '%1'").arg(node.access));

    node.initialValue = valueFromJson(json.value(QStringLiteral("initialValue")), node.dataType);
    node.arrayLength = json.value(QStringLiteral("arrayLength")).toInt(0);
    if (node.arrayLength < 0 || node.arrayLength > kMaxArrayLength)
        return fail(path, tr("array length %1 is outside 0..%2").arg(node.arrayLength).arg(kMaxArrayLength));

    const QString quality = json.value(QStringLiteral("quality")).toString(qualityKey(node.quality));
    if (!qualityFromKey(quality, node.quality))
        return fail(path, tr("unknown quality '%1'").arg(quality));

    node.historizing = json.value(QStringLiteral("historizing")).toBool(false);
    const QJsonObject properties = json.value(QStringLiteral("properties")).toObject();
    for (auto it = properties.constBegin(); it != properties.constEnd(); ++it)
        node.properties.insert(it.key(), it.value().toString());

    const QJsonObject simulation = json.value(QStringLiteral("simulation")).toObject();
    const QString function = simulation.value(QStringLiteral("function")).toString(QStringLiteral("none"));
    bool known = false;
    node.simulation.function = SignalGenerator::fromKey(function, &known);
    if (!known)
        return fail(path, tr("unknown simulation function '%1'").arg(function));
    node.simulation.intervalMs = simulation.value(QStringLiteral("intervalMs")).toInt(node.simulation.intervalMs);
    node.simulation.periodMs = simulation.value(QStringLiteral("periodMs")).toInt(node.simulation.periodMs);
    node.simulation.amplitude = simulation.value(QStringLiteral("amplitude")).toDouble();
    node.simulation.offset = simulation.value(QStringLiteral("offset")).toDouble();
    const QJsonArray values = simulation.value(QStringLiteral("values")).toArray();
    for (const QJsonValue &value : values)
        node.simulation.values.append(value.toString());
    if (json.contains(QStringLiteral("alarm"))) {
        node.alarm = alarmFromJson(json.value(QStringLiteral("alarm")).toObject());
        const QString problem = AlarmRules::problem(node.alarm, node.dataType, node.arrayLength);
        if (!problem.isEmpty())
            return fail(path, problem);
    }
    return true;
}

///
/// \brief Records why reading stopped.
/// \param path Location of the invalid node.
/// \param message Problem description.
/// \return Always false, so callers can return it directly.
///
bool NodeReader::fail(const QString &path, const QString &message)
{
    _error = QStringLiteral("%1: %2").arg(path, message);
    return false;
}

///
/// \brief Builds a failed load result.
/// \param error Reason.
/// \return Result carrying only the error.
///
ProjectFile::LoadResult failure(const QString &error)
{
    ProjectFile::LoadResult result;
    result.error = error;
    return result;
}

}

namespace ProjectFile {

///
/// \brief Returns the file name filter of project files for file dialogs.
/// \return Filter such as "OpenUaSim project (*.ouasim)".
///
QString fileFilter()
{
    return tr("OpenUaSim project (*.ouasim)");
}

///
/// \brief Serializes a project to JSON.
/// \param project Project to write.
/// \return Indented JSON document.
///
QByteArray toJson(const ProjectConfig &project)
{
    QJsonArray endpoints;
    for (const EndpointInfo &endpoint : project.endpoints)
        endpoints.append(endpointToJson(endpoint));

    QJsonArray users;
    for (const UserAccount &user : project.users)
        users.append(QJsonObject { { QStringLiteral("username"), user.username },
                                   { QStringLiteral("password"), user.password } });

    QJsonArray nodes;
    for (const NodeDefinition &node : project.nodes)
        nodes.append(nodeToJson(node));

    QJsonArray modelVariables;
    for (const ModelVariable &variable : project.modelVariables)
        modelVariables.append(modelVariableToJson(variable));

    const QJsonObject server {
        { QStringLiteral("applicationName"), project.server.applicationName },
        { QStringLiteral("applicationUri"), project.server.applicationUri },
        { QStringLiteral("maxSessions"), project.server.maxSessions },
        { QStringLiteral("historyMaxValues"), project.server.history.maxValues },
        { QStringLiteral("historyMaxAgeMinutes"), project.server.history.maxAgeMinutes },
    };

    const QJsonObject root {
        { QStringLiteral("formatVersion"), kFormatVersion },
        { QStringLiteral("server"), server },
        { QStringLiteral("endpoints"), endpoints },
        { QStringLiteral("users"), users },
        { QStringLiteral("nodes"), nodes },
        { QStringLiteral("nodeSetFiles"), QJsonArray::fromStringList(project.nodeSetFiles) },
        { QStringLiteral("modelVariables"), modelVariables },
    };
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

///
/// \brief Parses a project from JSON and validates it.
/// \param json Document produced by toJson().
/// \return Project, or the first problem found: malformed JSON, a newer format version,
///         an unknown node kind, data type, access level or simulation function, a duplicated
///         node id, or model variable settings without a namespace URI or given twice.
///
LoadResult fromJson(const QByteArray &json)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return failure(tr("Malformed project file: %1").arg(parseError.errorString()));
    if (!document.isObject())
        return failure(tr("Malformed project file: the root is not an object"));

    const QJsonObject root = document.object();
    const int version = root.value(QStringLiteral("formatVersion")).toInt(0);
    if (version < 1)
        return failure(tr("Not an OpenUaSim project file"));
    if (version > kFormatVersion)
        return failure(tr("The project was saved by a newer version of OpenUaSim (format %1)").arg(version));

    LoadResult result;
    ProjectConfig &project = result.project;

    const QJsonObject server = root.value(QStringLiteral("server")).toObject();
    project.server.applicationName =
        server.value(QStringLiteral("applicationName")).toString(project.server.applicationName);
    project.server.applicationUri =
        server.value(QStringLiteral("applicationUri")).toString(project.server.applicationUri);
    project.server.maxSessions = server.value(QStringLiteral("maxSessions")).toInt(project.server.maxSessions);
    HistoryLimits &history = project.server.history;
    history.maxValues = server.value(QStringLiteral("historyMaxValues")).toInt(history.maxValues);
    history.maxAgeMinutes = server.value(QStringLiteral("historyMaxAgeMinutes")).toInt(history.maxAgeMinutes);

    const QJsonArray users = root.value(QStringLiteral("users")).toArray();
    for (const QJsonValue &user : users)
        addUser(project.users, userFromJson(user.toObject()));

    const QJsonArray endpoints = root.value(QStringLiteral("endpoints")).toArray();
    for (const QJsonValue &endpoint : endpoints) {
        project.endpoints.append(endpointFromJson(endpoint.toObject()));
        // Format 1 kept one login on each endpoint; it joins the project's user accounts.
        addUser(project.users, userFromJson(endpoint.toObject()));
    }

    NodeReader reader;
    if (!reader.read(root.value(QStringLiteral("nodes")).toArray(), QStringLiteral("nodes"), project.nodes))
        return failure(tr("Invalid project file: %1").arg(reader.error()));

    const QJsonArray nodeSetFiles = root.value(QStringLiteral("nodeSetFiles")).toArray();
    for (const QJsonValue &file : nodeSetFiles)
        project.nodeSetFiles.append(file.toString());

    NodeReader modelReader;
    if (!modelReader.readModelVariables(root.value(QStringLiteral("modelVariables")).toArray(),
                                        QStringLiteral("modelVariables"), project.modelVariables))
        return failure(tr("Invalid project file: %1").arg(modelReader.error()));
    return result;
}

///
/// \brief Writes a project to a file, replacing it atomically.
/// \param project Project to write; its model paths are stored relative to the file's directory.
/// \param fileName Target path.
/// \param error Receives the reason on failure; may be null.
/// \return True when the file was written.
///
bool save(const ProjectConfig &project, const QString &fileName, QString *error)
{
    ProjectConfig stored = project;
    const QDir directory = QFileInfo(fileName).absoluteDir();
    for (QString &model : stored.nodeSetFiles)
        model = directory.relativeFilePath(model);

    QSaveFile file(fileName);
    if (file.open(QIODevice::WriteOnly) && file.write(toJson(stored)) >= 0 && file.commit())
        return true;
    if (error)
        *error = file.errorString();
    return false;
}

///
/// \brief Reads a project from a file.
/// \param fileName Path to read.
/// \return Project with absolute model paths, or the reason the file could not be opened or parsed.
///
LoadResult load(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        return failure(tr("Cannot open '%1': %2").arg(fileName, file.errorString()));
    LoadResult result = fromJson(file.readAll());
    const QDir directory = QFileInfo(fileName).absoluteDir();
    for (QString &model : result.project.nodeSetFiles)
        model = QDir::cleanPath(directory.absoluteFilePath(model));
    return result;
}

}
