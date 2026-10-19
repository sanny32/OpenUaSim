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
/// \return "folder", "object" or "variable".
///
QString kindKey(NodeDefinition::Kind kind)
{
    switch (kind) {
    case NodeDefinition::Kind::Folder:   return QStringLiteral("folder");
    case NodeDefinition::Kind::Object:   return QStringLiteral("object");
    case NodeDefinition::Kind::Variable: return QStringLiteral("variable");
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
                                            NodeDefinition::Kind::Variable }) {
        if (kindKey(candidate) == key) {
            kind = candidate;
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
    return {
        { QStringLiteral("function"), SignalGenerator::key(simulation.function) },
        { QStringLiteral("intervalMs"), simulation.intervalMs },
        { QStringLiteral("periodMs"), simulation.periodMs },
        { QStringLiteral("amplitude"), simulation.amplitude },
        { QStringLiteral("offset"), simulation.offset },
    };
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
        json.insert(QStringLiteral("simulation"), simulationToJson(node.simulation));
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
        { QStringLiteral("username"), endpoint.username },
        { QStringLiteral("password"), endpoint.password },
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
    endpoint.username = json.value(QStringLiteral("username")).toString();
    endpoint.password = json.value(QStringLiteral("password")).toString();
    endpoint.trustedClientsOnly =
        json.value(QStringLiteral("requireClientCertificate")).toBool(endpoint.trustedClientsOnly);
    return endpoint;
}

///
/// \brief Parses the node tree of a project and reports the first invalid node.
///
class NodeReader
{
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

    const QJsonArray endpoints = root.value(QStringLiteral("endpoints")).toArray();
    for (const QJsonValue &endpoint : endpoints)
        project.endpoints.append(endpointFromJson(endpoint.toObject()));

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
