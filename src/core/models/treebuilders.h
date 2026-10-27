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
/// \brief Returns the display name of a certificate store folder.
/// \param category Store folder.
/// \return Folder name.
///
QString certificateCategoryName(CertificateInfo::Category category);

///
/// \brief Returns the icon of a certificate store folder.
/// \param category Store folder.
/// \return Icon resource name.
///
QString certificateCategoryIcon(CertificateInfo::Category category);

///
/// \brief Returns the id of the node that lists the certificates of every folder.
/// \return Id that is not a category number.
///
QString allCertificatesId();

///
/// \brief Builds the certificate store folder list.
/// \param certificates Certificates of every folder.
/// \return One node per CertificateInfo::Category in declaration order with the category number as id, then a
///         node of every certificate with allCertificatesId() set apart by a separator; each has the certificate
///         count as trailing text.
///
QList<TreeNode> certificateCategories(const QList<CertificateInfo> &certificates);

}
