// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file treebuilders.h
/// \brief Declares the builders of the navigation trees derived from server data.
///

#pragma once

#include <QList>

#include "data/certificateinfo.h"
#include "data/treenode.h"

namespace TreeBuilders {

///
/// \brief Returns the id of the node that lists the certificates of every folder.
/// \return Id that is not a category number.
///
QString allCertificatesId();

///
/// \brief Builds the certificate store folder list.
/// \param certificates Certificates of every folder.
/// \return A node of every certificate with allCertificatesId(), then one node per CertificateInfo::Category
///         in declaration order with the category number as id; each has the certificate count as trailing text.
///
QList<TreeNode> certificateCategories(const QList<CertificateInfo> &certificates);

}
