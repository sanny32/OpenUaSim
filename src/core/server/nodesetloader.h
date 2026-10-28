// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file nodesetloader.h
/// \brief Declares the loading and export of NodeSet2 models on the served server.
///

#pragma once

#include <QHash>
#include <QObject>
#include <QSet>
#include <QStringList>

#include <QUaCustomDataTypes>

#include "data/logentry.h"

class SimServer;

///
/// \brief Loads NodeSet2 models into the server once each, remembers which namespaces they brought, and exports
///        served namespaces as NodeSet2.
///
class NodeSetLoader : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Creates a loader that has loaded nothing.
    /// \param server Server receiving the models; must outlive the loader.
    /// \param parent Owning QObject.
    ///
    explicit NodeSetLoader(SimServer &server, QObject *parent = nullptr);

    ///
    /// \brief Loads a NodeSet2 file into the server once and logs its outcome.
    /// \param fileName Absolute path.
    /// \param error Receives the reason on failure; may be null.
    /// \return True when the file is loaded, now or before.
    ///
    bool load(const QString &fileName, QString *error = nullptr);

    ///
    /// \brief Loads the files that are not loaded yet; failures go to the log.
    /// \param fileNames Absolute paths.
    ///
    void loadAll(const QStringList &fileNames);

    ///
    /// \brief Returns the files loaded into the current server.
    /// \return Absolute paths in load order.
    ///
    QStringList loaded() const;

    ///
    /// \brief Returns the number of nodes added by loaded models.
    ///
    int nodeCount() const;

    ///
    /// \brief Returns the loaded model each namespace came from.
    /// \return Model file keyed by namespace URI; namespaces 0 and 1 are not listed.
    ///
    QHash<QString, QString> modelNamespaces() const;

    ///
    /// \brief Points the loaded models at moved files without reloading them.
    /// \param movedFiles New absolute path keyed by the former one; paths not loaded are ignored.
    ///
    void renameFiles(const QHash<QString, QString> &movedFiles);

    ///
    /// \brief Forgets every loaded model, e.g. after the server was recreated without them.
    ///
    void clear();

    ///
    /// \brief Writes the nodes of the application namespace, i.e. the project nodes, as a NodeSet2 file.
    /// \param fileName Target path.
    /// \param error Receives the reason on failure; may be null.
    /// \return True when the file was written; values that cannot be exported are logged.
    ///
    bool exportApplicationNamespace(const QString &fileName, QString *error = nullptr);

    ///
    /// \brief Writes the nodes of one namespace as a NodeSet2 file.
    /// \param fileName Target path.
    /// \param namespaceUri URI of a served namespace other than the standard namespace 0.
    /// \param error Receives the reason on failure; may be null.
    /// \return True when the file was written; values that cannot be exported are logged.
    ///
    bool exportNamespace(const QString &fileName, const QString &namespaceUri, QString *error = nullptr);

signals:
    ///
    /// \brief Announces a model event for the server log.
    /// \param entry Log entry.
    ///
    void logAppended(const LogEntry &entry);

    ///
    /// \brief Announces the nodes a loaded model added to the server.
    /// \param nodeIds Added nodes in the order they were added.
    ///
    void nodesAdded(const QList<QUaNodeId> &nodeIds);

private:
    QStringList addedNamespaces(const QStringList &namespacesBefore) const;
    bool fail(const QString &fileName, const QString &reason, QString *error);
    void log(LogEntry::Level level, const QString &message);

    SimServer &_server;
    QStringList _loaded;
    QHash<QString, QString> _modelNamespaces;
    QSet<QString> _unclaimedNamespaces;
    int _nodeCount = 0;
};
