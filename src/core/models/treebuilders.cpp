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
/// \brief Builds an expanded session group.
/// \param title Group title, with a %1 placeholder for the session count.
/// \param icon Group icon resource name.
/// \param sessions Sessions of the group.
/// \return Group node with one child per session.
///
TreeNode sessionGroup(const QString &title, const QString &icon, const QList<SessionInfo> &sessions)
{
    TreeNode group;
    group.text = title.arg(sessions.size());
    group.icon = icon;
    group.kind = TreeNode::Kind::Category;
    group.expanded = true;
    for (const SessionInfo &session : sessions) {
        TreeNode child;
        child.text = session.name;
        child.icon = QStringLiteral("user");
        child.id = session.sessionId;
        child.kind = TreeNode::Kind::Session;
        group.children.append(child);
    }
    return group;
}

}

///
/// \brief Builds the namespace navigation tree.
/// \param namespaces Namespace array.
/// \return One expanded "All Namespaces" node listing "[index] uri" children keyed by URI.
///
QList<TreeNode> namespaceTree(const QList<NamespaceInfo> &namespaces)
{
    TreeNode root;
    root.text = QCoreApplication::translate("TreeBuilders", "All Namespaces");
    root.icon = QStringLiteral("database");
    root.kind = TreeNode::Kind::Category;
    root.expanded = true;
    for (const NamespaceInfo &ns : namespaces) {
        TreeNode child;
        child.text = QStringLiteral("[%1] %2").arg(ns.index).arg(ns.uri);
        child.icon = QStringLiteral("database");
        child.id = ns.uri;
        child.kind = TreeNode::Kind::Namespace;
        root.children.append(child);
    }
    return { root };
}

///
/// \brief Builds the flat endpoint list.
/// \param endpoints Configured endpoints.
/// \return One node per endpoint showing its name over its URL, keyed by URL.
///
QList<TreeNode> endpointList(const QList<EndpointInfo> &endpoints)
{
    QList<TreeNode> nodes;
    for (const EndpointInfo &endpoint : endpoints) {
        TreeNode node;
        node.text = endpoint.name + QLatin1Char('\n') + endpoint.url();
        node.icon = endpoint.icon;
        node.id = endpoint.url();
        node.kind = TreeNode::Kind::Endpoint;
        nodes.append(node);
    }
    return nodes;
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

///
/// \brief Builds the session navigation tree.
/// \param sessions Active and closed sessions.
/// \return Expanded "Active Sessions (n)" and "Closed Sessions (n)" groups whose
///         children are keyed by session id.
///
QList<TreeNode> sessionTree(const QList<SessionInfo> &sessions)
{
    QList<SessionInfo> active;
    QList<SessionInfo> closed;
    for (const SessionInfo &session : sessions)
        (session.active ? active : closed).append(session);

    return {
        sessionGroup(QCoreApplication::translate("TreeBuilders", "Active Sessions (%1)"),
                     QStringLiteral("status-good"), active),
        sessionGroup(QCoreApplication::translate("TreeBuilders", "Closed Sessions (%1)"),
                     QStringLiteral("status-inactive"), closed),
    };
}

}
