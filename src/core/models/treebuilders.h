// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file treebuilders.h
/// \brief Declares the builders of the navigation trees derived from server data.
///

#pragma once

#include <QList>

#include "data/certificateinfo.h"
#include "data/endpointinfo.h"
#include "data/namespaceinfo.h"
#include "data/sessioninfo.h"
#include "data/treenode.h"

namespace TreeBuilders {

///
/// \brief Builds the namespace navigation tree.
/// \param namespaces Namespace array.
/// \return One expanded "All Namespaces" node listing "[index] uri" children keyed by URI.
///
QList<TreeNode> namespaceTree(const QList<NamespaceInfo> &namespaces);

///
/// \brief Builds the flat endpoint list.
/// \param endpoints Configured endpoints.
/// \return One node per endpoint showing its name over its URL, keyed by URL.
///
QList<TreeNode> endpointList(const QList<EndpointInfo> &endpoints);

///
/// \brief Builds the certificate store folder list.
/// \param certificates Certificates of every folder.
/// \return One node per CertificateInfo::Category, in declaration order, with the
///         certificate count as trailing text and the category number as id.
///
QList<TreeNode> certificateCategories(const QList<CertificateInfo> &certificates);

///
/// \brief Builds the session navigation tree.
/// \param sessions Active and closed sessions.
/// \return Expanded "Active Sessions (n)" and "Closed Sessions (n)" groups whose
///         children are keyed by session id.
///
QList<TreeNode> sessionTree(const QList<SessionInfo> &sessions);

}
