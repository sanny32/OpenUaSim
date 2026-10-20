// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectfile.cpp
/// \brief Implements reading and writing of OpenUaSim project files.
///

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>

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
/// \return "good", "uncertain" or "bad".
///
QString qualityKey(VariableInfo::Quality quality)
{
    switch (quality) {
    case VariableInfo::Quality::Good:      return QStringLiteral("good");
    case VariableInfo::Quality::Uncertain: return QStringLiteral("uncertain");
    case VariableInfo::Quality::Bad:       return QStringLiteral("bad");
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
    for (VariableInfo::Quality candidate : { VariableInfo::Quality::Good, VariableInfo::Quality::Uncertain,
                                             VariableInfo::Quality::Bad }) {
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
    if (node.kind == NodeDefinition::Kind::Variable) {
        json.insert(QStringLiteral("dataType"), node.dataType);
        json.insert(QStringLiteral("access"), node.access);
        json.insert(QStringLiteral("initialValue"), valueToJson(node.initialValue));
        if (node.arrayLength > 0)
            json.insert(QStringLiteral("arrayLength"), node.arrayLength);
        json.insert(QStringLiteral("quality"), qualityKey(node.quality));
        json.insert(QStringLiteral("simulation"), simulationToJson(node.simulation));
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
/// \return False for an unsupported data type, access level or simulation function.
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

    const QJsonObject server {
        { QStringLiteral("applicationName"), project.server.applicationName },
        { QStringLiteral("applicationUri"), project.server.applicationUri },
        { QStringLiteral("maxSessions"), project.server.maxSessions },
    };

    const QJsonObject root {
        { QStringLiteral("formatVersion"), kFormatVersion },
        { QStringLiteral("server"), server },
        { QStringLiteral("endpoints"), endpoints },
        { QStringLiteral("users"), users },
        { QStringLiteral("nodes"), nodes },
        { QStringLiteral("nodeSetFiles"), QJsonArray::fromStringList(project.nodeSetFiles) },
    };
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

///
/// \brief Parses a project from JSON and validates it.
/// \param json Document produced by toJson().
/// \return Project, or the first problem found: malformed JSON, a newer format version,
///         an unknown node kind, data type, access level or simulation function, or a
///         duplicated node id.
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
    return result;
}

///
/// \brief Writes a project to a file, replacing it atomically.
/// \param project Project to write.
/// \param fileName Target path.
/// \param error Receives the reason on failure; may be null.
/// \return True when the file was written.
///
bool save(const ProjectConfig &project, const QString &fileName, QString *error)
{
    QSaveFile file(fileName);
    if (file.open(QIODevice::WriteOnly) && file.write(toJson(project)) >= 0 && file.commit())
        return true;
    if (error)
        *error = file.errorString();
    return false;
}

///
/// \brief Reads a project from a file.
/// \param fileName Path to read.
/// \return Project, or the reason the file could not be opened or parsed.
///
LoadResult load(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        return failure(tr("Cannot open '%1': %2").arg(fileName, file.errorString()));
    return fromJson(file.readAll());
}

}
