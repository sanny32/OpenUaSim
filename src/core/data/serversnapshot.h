// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file serversnapshot.h
/// \brief Declares the complete state the UI displays.
///

#pragma once

#include <QHash>
#include <QList>

#include "certificateinfo.h"
#include "endpointinfo.h"
#include "logentry.h"
#include "namespaceinfo.h"
#include "serverstatus.h"
#include "sessioninfo.h"
#include "treenode.h"
#include "typeinfo.h"
#include "variableinfo.h"

///
/// \brief Everything the main window shows, gathered from one source.
///
struct ServerSnapshot
{
    /// \brief Server summary.
    ServerStatus status;
    /// \brief Address space tree.
    QList<TreeNode> addressSpace;
    /// \brief Variables of each object, keyed by the object node id.
    QHash<QString, QList<VariableInfo>> variables;
    /// \brief Types keyed by node id.
    QHash<QString, TypeInfo> types;
    /// \brief Namespace array.
    QList<NamespaceInfo> namespaces;
    /// \brief Configured endpoints.
    QList<EndpointInfo> endpoints;
    /// \brief Certificates of every store folder.
    QList<CertificateInfo> certificates;
    /// \brief Active and closed sessions.
    QList<SessionInfo> sessions;
    /// \brief Server log.
    QList<LogEntry> log;
};
