// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file nodesetwriter.h
/// \brief Declares the export of project nodes as an OPC UA NodeSet2 document.
///

#pragma once

#include <QByteArray>
#include <QList>
#include <QStringList>

#include "data/projectconfig.h"

namespace NodeSetWriter {

///
/// \brief Returns the file name filter of NodeSet2 files for file dialogs.
/// \return Filter such as "NodeSet2 XML (*.xml)".
///
QString fileFilter();

///
/// \brief Writes project nodes as a NodeSet2 document.
/// \param nodes Node tree with node ids; top-level nodes are organized by the Objects folder.
/// \param namespaceUris URIs of namespace indexes 1, 2, ... used by the node ids.
/// \return UTF-8 XML with folders and objects as UAObject and variables as UAVariable carrying
///         their data type, access level and initial value.
///
QByteArray write(const QList<NodeDefinition> &nodes, const QStringList &namespaceUris);

///
/// \brief Writes project nodes to a NodeSet2 file, replacing it atomically.
/// \param nodes Node tree with node ids.
/// \param namespaceUris URIs of namespace indexes 1, 2, ... used by the node ids.
/// \param fileName Target path.
/// \param error Receives the reason on failure; may be null.
/// \return True when the file was written.
///
bool save(const QList<NodeDefinition> &nodes, const QStringList &namespaceUris, const QString &fileName,
          QString *error = nullptr);

}
