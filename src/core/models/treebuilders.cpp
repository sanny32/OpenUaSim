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

///
/// \brief Builds a category node.
/// \param text Display name.
/// \param count Number of certificates it lists.
/// \param icon Icon resource name.
/// \param id Selection id.
/// \return Node with the count as trailing text.
///
TreeNode categoryNode(const QString &text, qsizetype count, const QString &icon, const QString &id)
{
    TreeNode node;
    node.text = text;
    node.trailing = QString::number(count);
    node.icon = icon;
    node.id = id;
    node.kind = TreeNode::Kind::Category;
    return node;
}

}

///
/// \brief Returns the id of the node that lists the certificates of every folder.
/// \return Id that is not a category number.
///
QString allCertificatesId()
{
    return QStringLiteral("all");
}

///
/// \brief Builds the certificate store folder list.
/// \param certificates Certificates of every folder.
/// \return A node of every certificate with allCertificatesId(), then one node per CertificateInfo::Category
///         in declaration order with the category number as id; each has the certificate count as trailing text.
///
QList<TreeNode> certificateCategories(const QList<CertificateInfo> &certificates)
{
    const CertificateInfo::Category categories[] = {
        CertificateInfo::Category::Application,
        CertificateInfo::Category::Trusted,
        CertificateInfo::Category::Rejected,
        CertificateInfo::Category::Issuer,
    };

    QList<TreeNode> nodes { categoryNode(QCoreApplication::translate("TreeBuilders", "All Certificates"),
                                         certificates.size(), QStringLiteral("list"), allCertificatesId()) };
    for (CertificateInfo::Category category : categories) {
        const auto count = std::count_if(certificates.cbegin(), certificates.cend(),
                                         [category](const CertificateInfo &certificate) {
                                             return certificate.category == category;
                                         });
        nodes.append(categoryNode(categoryName(category), count, categoryIcon(category),
                                  QString::number(static_cast<int>(category))));
    }
    return nodes;
}

}
