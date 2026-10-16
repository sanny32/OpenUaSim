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
/// \brief Builds the certificate store folder list.
/// \param certificates Certificates of every folder.
/// \return One node per CertificateInfo::Category, in declaration order, with the
///         certificate count as trailing text and the category number as id.
///
QList<TreeNode> certificateCategories(const QList<CertificateInfo> &certificates);

}
