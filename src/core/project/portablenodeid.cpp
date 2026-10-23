// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file portablenodeid.cpp
/// \brief Implements node ids that name their namespace by URI, so they stay valid when models load in another order.
///

#include <QRegularExpression>

#include "portablenodeid.h"

namespace {

///
/// \brief Tells whether a text is an identifier part of a node id in XML notation.
/// \param identifier Text after the namespace part.
/// \return True for forms such as "i=1001" or "s=Name".
///
bool isIdentifier(const QString &identifier)
{
    static const QRegularExpression pattern(QStringLiteral("^[isgb]=.+$"));
    return pattern.match(identifier).hasMatch();
}

}

namespace PortableNodeId {

///
/// \brief Replaces the namespace index of a server node id with the namespace URI.
/// \param nodeId Server node id, e.g. "ns=2;i=1001" or "i=2368".
/// \param namespaces Namespace URIs of the server; the position of a URI is its index.
/// \return Text such as "nsu=urn:machines;i=1001", the node id itself in namespace 0, or empty for a malformed
///         node id or an unknown namespace index.
///
QString fromServer(const QString &nodeId, const QStringList &namespaces)
{
    static const QRegularExpression pattern(QStringLiteral("^ns=(\\d+);(.+)$"));
    const QRegularExpressionMatch match = pattern.match(nodeId);
    const int index = match.hasMatch() ? match.captured(1).toInt() : 0;
    const QString identifier = match.hasMatch() ? match.captured(2) : nodeId;
    if (!isIdentifier(identifier) || index >= namespaces.size())
        return {};
    return index == 0 ? identifier : QStringLiteral("nsu=%1;%2").arg(namespaces.at(index), identifier);
}

///
/// \brief Replaces the namespace URI of a portable node id with the index the server gives it.
/// \param portable Text as fromServer() returns it.
/// \param namespaces Namespace URIs of the server; the position of a URI is its index.
/// \return Server node id such as "ns=2;i=1001" or "i=2368", or empty for malformed text or a namespace the
///         server does not have.
///
QString toServer(const QString &portable, const QStringList &namespaces)
{
    static const QRegularExpression pattern(QStringLiteral("^nsu=([^;]+);(.+)$"));
    const QRegularExpressionMatch match = pattern.match(portable);
    if (!match.hasMatch())
        return isIdentifier(portable) ? portable : QString();
    const qsizetype index = namespaces.indexOf(match.captured(1));
    if (index < 0 || !isIdentifier(match.captured(2)))
        return {};
    return index == 0 ? match.captured(2) : QStringLiteral("ns=%1;%2").arg(index).arg(match.captured(2));
}

}
