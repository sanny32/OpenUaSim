// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file treebuilders.cpp
/// \brief Implements the builders of the navigation trees derived from server data.
///

#include <algorithm>

#include <QCoreApplication>

#include "treebuilders.h"

namespace TreeBuilders {

namespace {

///
/// \brief Returns the display name of a certificate store folder.
/// \param category Store folder.
/// \return Folder name.
///
QString categoryName(CertificateInfo::Category category)
{
    switch (category) {
    case CertificateInfo::Category::Application:
        return QCoreApplication::translate("TreeBuilders", "Application Certificate");
    case CertificateInfo::Category::Trusted:
        return QCoreApplication::translate("TreeBuilders", "Trusted Clients");
    case CertificateInfo::Category::Rejected:
        return QCoreApplication::translate("TreeBuilders", "Rejected Certificates");
    case CertificateInfo::Category::Issuer:
        return QCoreApplication::translate("TreeBuilders", "Issuers");
    }
    return {};
}

///
/// \brief Returns the icon of a certificate store folder.
/// \param category Store folder.
/// \return Icon resource name.
///
QString categoryIcon(CertificateInfo::Category category)
{
    switch (category) {
    case CertificateInfo::Category::Application: return QStringLiteral("settings");
    case CertificateInfo::Category::Trusted:     return QStringLiteral("shield-check");
    case CertificateInfo::Category::Rejected:    return QStringLiteral("shield-x");
    case CertificateInfo::Category::Issuer:      return QStringLiteral("certificate");
    }
    return {};
}

}

///
/// \brief Builds the certificate store folder list.
/// \param certificates Certificates of every folder.
/// \return One node per CertificateInfo::Category, in declaration order, with the
///         certificate count as trailing text and the category number as id.
///
QList<TreeNode> certificateCategories(const QList<CertificateInfo> &certificates)
{
    const CertificateInfo::Category categories[] = {
        CertificateInfo::Category::Application,
        CertificateInfo::Category::Trusted,
        CertificateInfo::Category::Rejected,
        CertificateInfo::Category::Issuer,
    };

    QList<TreeNode> nodes;
    for (CertificateInfo::Category category : categories) {
        const auto count = std::count_if(certificates.cbegin(), certificates.cend(),
                                         [category](const CertificateInfo &certificate) {
                                             return certificate.category == category;
                                         });
        TreeNode node;
        node.text = categoryName(category);
        node.trailing = QString::number(count);
        node.icon = categoryIcon(category);
        node.id = QString::number(static_cast<int>(category));
        node.kind = TreeNode::Kind::Category;
        nodes.append(node);
    }
    return nodes;
}

}
