// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file nodesetloader.cpp
/// \brief Implements the loading and export of NodeSet2 models on the served server.
///

#include <QUaServer>

#include "logmapper.h"
#include "nodesetloader.h"
#include "simserver.h"

///
/// \brief Creates a loader that has loaded nothing.
/// \param server Server receiving the models; must outlive the loader.
/// \param parent Owning QObject.
///
NodeSetLoader::NodeSetLoader(SimServer &server, QObject *parent)
    : QObject(parent)
    , _server(server)
{
}

///
/// \brief Loads a NodeSet2 file into the server once and logs its outcome.
/// \param fileName Absolute path.
/// \param error Receives the reason on failure; may be null.
/// \return True when the file is loaded, now or before.
///
bool NodeSetLoader::load(const QString &fileName, QString *error)
{
    if (_loaded.contains(fileName))
        return true;
    QUaServer *server = _server.uaServer();
    const QStringList namespacesBefore = server->namespaces();
    const QUaNodeSetResult result = server->loadNodeSet(fileName);
    if (!result.isOk()) {
        log(LogEntry::Level::Error, tr("Model '%1' not loaded: %2").arg(fileName, result.errorString));
        if (error)
            *error = result.errorString;
        return false;
    }
    for (const QString &warning : result.warnings)
        log(LogEntry::Level::Warning, tr("Model '%1': %2").arg(fileName, warning));
    log(LogEntry::Level::Info, tr("Model '%1' loaded with %2 nodes").arg(fileName).arg(result.addedNodes.size()));
    _loaded.append(fileName);
    for (const QString &uri : server->namespaces()) {
        if (!namespacesBefore.contains(uri))
            _modelNamespaces.insert(uri, fileName);
    }
    return true;
}

///
/// \brief Loads the files that are not loaded yet; failures go to the log.
/// \param fileNames Absolute paths.
///
void NodeSetLoader::loadAll(const QStringList &fileNames)
{
    for (const QString &fileName : fileNames)
        load(fileName);
}

///
/// \brief Returns the files loaded into the current server.
/// \return Absolute paths in load order.
///
QStringList NodeSetLoader::loaded() const
{
    return _loaded;
}

///
/// \brief Returns the loaded model each namespace came from.
/// \return Model file keyed by namespace URI; namespaces 0 and 1 are not listed.
///
QHash<QString, QString> NodeSetLoader::modelNamespaces() const
{
    return _modelNamespaces;
}

///
/// \brief Points the loaded models at moved files without reloading them.
/// \param movedFiles New absolute path keyed by the former one; paths not loaded are ignored.
///
void NodeSetLoader::renameFiles(const QHash<QString, QString> &movedFiles)
{
    for (QString &file : _loaded)
        file = movedFiles.value(file, file);
    for (QString &file : _modelNamespaces)
        file = movedFiles.value(file, file);
}

///
/// \brief Forgets every loaded model, e.g. after the server was recreated without them.
///
void NodeSetLoader::clear()
{
    _loaded.clear();
    _modelNamespaces.clear();
}

///
/// \brief Writes the nodes of the application namespace, i.e. the project nodes, as a NodeSet2 file.
/// \param fileName Target path.
/// \param error Receives the reason on failure; may be null.
/// \return True when the file was written; values that cannot be exported are logged.
///
bool NodeSetLoader::exportApplicationNamespace(const QString &fileName, QString *error)
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
bool NodeSetLoader::exportNamespace(const QString &fileName, const QString &namespaceUri, QString *error)
{
    QUaServer *server = _server.uaServer();
    if (namespaceUri.isEmpty() || namespaceUri == server->namespaces().value(0)) {
        if (error)
            *error = tr("The standard OPC UA namespace cannot be exported");
        return false;
    }
    const QUaNodeSetExportResult result = server->exportNodeSet(fileName, { namespaceUri });
    for (const QString &warning : result.warnings)
        log(LogEntry::Level::Warning, tr("Model export: %1").arg(warning));
    if (!result.isOk() && error)
        *error = result.errorString;
    return result.isOk();
}

///
/// \brief Announces a log entry of the server category.
/// \param level Severity.
/// \param message Text of the entry.
///
void NodeSetLoader::log(LogEntry::Level level, const QString &message)
{
    emit logAppended(LogMapper::entry(level, LogEntry::Category::Server, message));
}
