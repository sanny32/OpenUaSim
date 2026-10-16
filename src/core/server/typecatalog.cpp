// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file typecatalog.cpp
/// \brief Implements the standard types the simulator's nodes are instances of.
///

#include <QCoreApplication>

#include "typecatalog.h"

namespace {

const QString kBaseObjectType = QStringLiteral("i=58");
const QString kFolderType = QStringLiteral("i=61");
const QString kBaseVariableType = QStringLiteral("i=62");
const QString kBaseDataVariableType = QStringLiteral("i=63");
const QString kPropertyType = QStringLiteral("i=68");

///
/// \brief Describes one standard type.
/// \param name Browse name.
/// \param nodeId Node id.
/// \param baseTypeId Supertype node id; empty for a root.
/// \param category "ObjectType" or "VariableType".
/// \param isAbstract Whether the type is abstract.
/// \param description Description text.
/// \return Type without references.
///
TypeInfo type(const QString &name, const QString &nodeId, const QString &baseTypeId, const QString &category,
              bool isAbstract, const char *description)
{
    TypeInfo info;
    info.browseName = name;
    info.nodeId = nodeId;
    info.baseTypeId = baseTypeId;
    info.category = category;
    info.isAbstract = isAbstract;
    info.description = QCoreApplication::translate("TypeCatalog", description);
    return info;
}

///
/// \brief Builds the tree node of a type.
/// \param info Type.
/// \return Collapsed type node.
///
TreeNode typeNode(const TypeInfo &info)
{
    TreeNode node;
    node.text = info.browseName;
    node.icon = QStringLiteral("folder");
    node.id = info.nodeId;
    node.kind = TreeNode::Kind::Type;
    return node;
}

}

namespace TypeCatalog {

///
/// \brief Returns the standard object and variable types of namespace 0 the project nodes use.
/// \return Types keyed by node id, with HasSubtype counts.
///
QHash<QString, TypeInfo> types()
{
    const QString objectType = QStringLiteral("ObjectType");
    const QString variableType = QStringLiteral("VariableType");
    const QList<TypeInfo> list = {
        type(QStringLiteral("BaseObjectType"), kBaseObjectType, QString(), objectType, false,
             QT_TRANSLATE_NOOP("TypeCatalog", "Base type for all objects.")),
        type(QStringLiteral("FolderType"), kFolderType, kBaseObjectType, objectType, false,
             QT_TRANSLATE_NOOP("TypeCatalog", "Type for folder objects that organize other nodes.")),
        type(QStringLiteral("BaseVariableType"), kBaseVariableType, QString(), variableType, true,
             QT_TRANSLATE_NOOP("TypeCatalog", "Base type for all variables.")),
        type(QStringLiteral("BaseDataVariableType"), kBaseDataVariableType, kBaseVariableType, variableType, false,
             QT_TRANSLATE_NOOP("TypeCatalog", "Type for variables that hold data.")),
        type(QStringLiteral("PropertyType"), kPropertyType, kBaseVariableType, variableType, false,
             QT_TRANSLATE_NOOP("TypeCatalog", "Type for variables that describe a node.")),
    };

    QHash<QString, TypeInfo> result;
    for (const TypeInfo &info : list) {
        int subtypes = 0;
        for (const TypeInfo &other : list)
            subtypes += other.baseTypeId == info.nodeId ? 1 : 0;
        TypeInfo entry = info;
        entry.references = { { QStringLiteral("HasSubtype"), subtypes } };
        result.insert(entry.nodeId, entry);
    }
    return result;
}

///
/// \brief Builds the navigation tree of types().
/// \return One "Types" root holding the BaseObjectType and BaseVariableType hierarchies,
///         with FolderType current.
///
QList<TreeNode> tree()
{
    const QHash<QString, TypeInfo> all = types();

    TreeNode folder = typeNode(all.value(kFolderType));
    folder.current = true;

    TreeNode objectRoot = typeNode(all.value(kBaseObjectType));
    objectRoot.expanded = true;
    objectRoot.children = { folder };

    TreeNode variableRoot = typeNode(all.value(kBaseVariableType));
    variableRoot.expanded = true;
    variableRoot.children = { typeNode(all.value(kBaseDataVariableType)), typeNode(all.value(kPropertyType)) };

    TreeNode root;
    root.text = QCoreApplication::translate("TypeCatalog", "Types");
    root.icon = QStringLiteral("folder");
    root.kind = TreeNode::Kind::Folder;
    root.expanded = true;
    root.children = { objectRoot, variableRoot };
    return { root };
}

}
