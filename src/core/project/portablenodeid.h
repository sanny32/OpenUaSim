// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file portablenodeid.h
/// \brief Declares node ids that name their namespace by URI, so they stay valid when models load in another order.
///

#pragma once

#include <QString>
#include <QStringList>

namespace PortableNodeId {

///
/// \brief Replaces the namespace index of a server node id with the namespace URI.
/// \param nodeId Server node id, e.g. "ns=2;i=1001" or "i=2368".
/// \param namespaces Namespace URIs of the server; the position of a URI is its index.
/// \return Text such as "nsu=urn:machines;i=1001", the node id itself in namespace 0, or empty for a malformed
///         node id or an unknown namespace index.
///
QString fromServer(const QString &nodeId, const QStringList &namespaces);

///
/// \brief Replaces the namespace URI of a portable node id with the index the server gives it.
/// \param portable Text as fromServer() returns it.
/// \param namespaces Namespace URIs of the server; the position of a URI is its index.
/// \return Server node id such as "ns=2;i=1001" or "i=2368", or empty for malformed text or a namespace the
///         server does not have.
///
QString toServer(const QString &portable, const QStringList &namespaces);

}
