// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file typecatalog.cpp
/// \brief Implements the type hierarchy of the server: standard types and the types of imported models.
///

#include <QCoreApplication>
#include <QMap>
#include <QSet>

#include <QUaServer>

#include <algorithm>

#include "simulation/valuecast.h"
#include "typecatalog.h"

namespace {

const QString kFolderType = QStringLiteral("i=61");
const QString kBaseObjectType = QStringLiteral("i=58");
const QString kBaseDataVariableType = QStringLiteral("i=63");
const QString kDataItemType = QStringLiteral("i=2365");
const QString kBaseEventType = QStringLiteral("i=2041");

///
/// \brief A hierarchy of the type system, walked down from its root type.
///
struct RootType
{
    /// \brief Numeric node id of the root in namespace 0.
    quint32 nodeId;
    /// \brief Category of the types below the root.
    const char *category;
};

const RootType kRootTypes[] = {
    { UA_NS0ID_BASEOBJECTTYPE, "ObjectType" },
    { UA_NS0ID_BASEVARIABLETYPE, "VariableType" },
    { UA_NS0ID_BASEDATATYPE, "DataType" },
    { UA_NS0ID_REFERENCES, "ReferenceType" },
};

///
/// \brief Returns the text form of a node id, without the namespace of namespace 0.
/// \param nodeId Node id.
/// \return Text such as "i=61" or "ns=2;i=1000".
///
QString idText(const QUaNodeId &nodeId)
{
    QString text = nodeId.toXmlString();
    if (text.startsWith(QLatin1String("ns=0;")))
        text.remove(0, 5);
    return text;
}

///
/// \brief Returns the description of a type the project nodes use, which namespace 0 leaves empty.
/// \param nodeId Type node id.
/// \return Translated description, or empty for other types.
///
QString standardDescription(const QString &nodeId)
{
    static const QHash<QString, const char *> descriptions {
        { QStringLiteral("i=58"), QT_TRANSLATE_NOOP("TypeCatalog", "Base type for all objects.") },
        { QStringLiteral("i=61"), QT_TRANSLATE_NOOP("TypeCatalog", "Type for folder objects that organize other nodes.") },
        { QStringLiteral("i=62"), QT_TRANSLATE_NOOP("TypeCatalog", "Base type for all variables.") },
        { QStringLiteral("i=63"), QT_TRANSLATE_NOOP("TypeCatalog", "Type for variables that hold data.") },
        { QStringLiteral("i=68"), QT_TRANSLATE_NOOP("TypeCatalog", "Type for variables that describe a node.") },
    };
    const char *description = descriptions.value(nodeId);
    return description ? QCoreApplication::translate("TypeCatalog", description) : QString();
}

///
/// \brief Reads types from a server, remembering the names of the reference types it meets.
///
class TypeReader
{
public:
    explicit TypeReader(const QUaServer &server);
    void read(const QUaNodeId &typeId, const QString &baseTypeId, const QString &category,
              QHash<QString, TypeInfo> &types);

private:
    QString referenceName(const QUaNodeId &referenceTypeId);

    const QUaServer &_server;
    QHash<QString, QString> _referenceNames;
};

///
/// \brief Prepares reading the types of a server.
/// \param server Server holding the types.
///
TypeReader::TypeReader(const QUaServer &server)
    : _server(server)
{
}

///
/// \brief Reads a type and, through its HasSubtype references, every type derived from it.
/// \param typeId Type to read.
/// \param baseTypeId Text node id of its supertype; empty for a root.
/// \param category Category of the hierarchy, e.g. "DataType".
/// \param types Receives the types keyed by node id; a type already read is skipped.
///
void TypeReader::read(const QUaNodeId &typeId, const QString &baseTypeId, const QString &category,
                      QHash<QString, TypeInfo> &types)
{
    static const QUaNodeId hasSubtype(0, quint32(UA_NS0ID_HASSUBTYPE));
    static const QSet<QUaNodeId> childReferences { QUaNodeId(0, quint32(UA_NS0ID_HASCOMPONENT)),
                                                   QUaNodeId(0, quint32(UA_NS0ID_HASORDEREDCOMPONENT)),
                                                   QUaNodeId(0, quint32(UA_NS0ID_HASPROPERTY)) };
    const QString id = idText(typeId);
    if (types.contains(id))
        return;

    TypeInfo info;
    info.browseName = _server.browseName(typeId).name();
    info.nodeId = id;
    info.baseTypeId = baseTypeId;
    info.category = category;
    info.isAbstract = _server.isAbstract(typeId);
    info.description = _server.description(typeId).text();
    if (info.description.isEmpty())
        info.description = standardDescription(id);

    QList<QUaNodeId> subtypes;
    QMap<QString, int> counts;
    const QList<QUaNodeReference> references = _server.references(typeId);
    for (const QUaNodeReference &reference : references) {
        if (!reference.isForward)
            continue;
        if (reference.referenceTypeId == hasSubtype)
            subtypes.append(reference.targetNodeId);
        if (childReferences.contains(reference.referenceTypeId))
            ++info.children;
        ++counts[referenceName(reference.referenceTypeId)];
    }
    for (auto it = counts.cbegin(); it != counts.cend(); ++it)
        info.references.append({ it.key(), it.value() });
    types.insert(id, info);

    for (const QUaNodeId &subtype : std::as_const(subtypes))
        read(subtype, id, category, types);
}

///
/// \brief Returns the browse name of a reference type.
/// \param referenceTypeId Reference type node id.
/// \return Name such as "HasSubtype".
///
QString TypeReader::referenceName(const QUaNodeId &referenceTypeId)
{
    const QString id = idText(referenceTypeId);
    auto found = _referenceNames.constFind(id);
    if (found == _referenceNames.cend())
        found = _referenceNames.insert(id, _server.browseName(referenceTypeId).name());
    return *found;
}

///
/// \brief Tells whether a type is another type or derives from it.
/// \param types Types keyed by node id.
/// \param typeId Type to check.
/// \param superTypeId Possible supertype.
/// \return True when the supertype chain of typeId reaches superTypeId.
///
bool derivesFrom(const QHash<QString, TypeInfo> &types, const QString &typeId, const QString &superTypeId)
{
    for (QString id = typeId; !id.isEmpty(); id = types.value(id).baseTypeId) {
        if (id == superTypeId)
            return true;
    }
    return false;
}

///
/// \brief Describes a type for the navigation tree.
/// \param info Type to describe.
/// \return Tree node without children.
///
TreeNode typeNode(const TypeInfo &info)
{
    TreeNode node;
    node.text = info.browseName;
    node.icon = QStringLiteral("folder");
    node.id = info.nodeId;
    node.kind = TreeNode::Kind::Type;
    node.current = info.nodeId == kFolderType;
    return node;
}

///
/// \brief Builds the tree nodes of the shown subtypes of a type, sorted by name.
/// \param parentId Type whose subtypes are built.
/// \param subtypes Types keyed by the node id of their supertype.
/// \param shown Node ids of the types to show.
/// \return Tree nodes with their own shown subtypes.
///
QList<TreeNode> subtypeNodes(const QString &parentId, const QMultiHash<QString, const TypeInfo *> &subtypes,
                             const QSet<QString> &shown)
{
    QList<const TypeInfo *> children = subtypes.values(parentId);
    std::sort(children.begin(), children.end(), [](const TypeInfo *a, const TypeInfo *b) {
        return a->browseName.compare(b->browseName, Qt::CaseInsensitive) < 0;
    });
    QList<TreeNode> nodes;
    for (const TypeInfo *child : std::as_const(children)) {
        if (!shown.contains(child->nodeId))
            continue;
        TreeNode node = typeNode(*child);
        node.children = subtypeNodes(child->nodeId, subtypes, shown);
        nodes.append(node);
    }
    return nodes;
}

}

namespace TypeCatalog {

///
/// \brief Reads the object, variable, data and reference types of a server.
/// \param server Server holding the standard types and the types of imported models.
/// \return Types keyed by node id, e.g. "i=61" or "ns=2;i=1000", with their supertype, category, declared
///         children and forward references counted by reference type.
///
QHash<QString, TypeInfo> types(const QUaServer &server)
{
    QHash<QString, TypeInfo> result;
    TypeReader reader(server);
    for (const RootType &root : kRootTypes)
        reader.read(QUaNodeId(0, root.nodeId), QString(), QString::fromLatin1(root.category), result);
    return result;
}

///
/// \brief Tells whether a type is shown without the full standard type system.
/// \param type Type to check.
/// \return True for a type outside namespace 0, a root of a hierarchy, a type of the project nodes or a data type
///         the simulation writes.
///
bool isRelevant(const TypeInfo &type)
{
    static const QSet<QString> projectTypes { QStringLiteral("i=58"), kFolderType, QStringLiteral("i=62"),
                                              QStringLiteral("i=63"), QStringLiteral("i=68") };
    return type.nodeId.startsWith(QLatin1String("ns=")) || type.baseTypeId.isEmpty()
        || projectTypes.contains(type.nodeId)
        || (type.category == QLatin1String("DataType") && ValueCast::isSupported(type.browseName));
}

///
/// \brief Builds the navigation tree of types.
/// \param types Types keyed by node id, as types() returns them.
/// \param allStandardTypes Whether every type is shown; otherwise only relevant types and their supertypes.
/// \return One "Types" root holding a hierarchy per root type, siblings sorted by name, with FolderType current.
///
QList<TreeNode> tree(const QHash<QString, TypeInfo> &types, bool allStandardTypes)
{
    QSet<QString> shown;
    QMultiHash<QString, const TypeInfo *> subtypes;
    QList<const TypeInfo *> roots;
    for (const TypeInfo &type : types) {
        if (types.contains(type.baseTypeId))
            subtypes.insert(type.baseTypeId, &type);
        else
            roots.append(&type);
        if (!allStandardTypes && !isRelevant(type))
            continue;
        for (QString id = type.nodeId; !id.isEmpty() && !shown.contains(id); id = types.value(id).baseTypeId)
            shown.insert(id);
    }
    const auto rootOrder = [](const TypeInfo *type) {
        const auto *found = std::find_if(std::cbegin(kRootTypes), std::cend(kRootTypes), [type](const RootType &root) {
            return type->nodeId == idText(QUaNodeId(0, root.nodeId));
        });
        return int(std::distance(std::cbegin(kRootTypes), found));
    };
    std::sort(roots.begin(), roots.end(), [&rootOrder](const TypeInfo *a, const TypeInfo *b) {
        const int orderA = rootOrder(a);
        const int orderB = rootOrder(b);
        return orderA != orderB ? orderA < orderB : a->browseName.compare(b->browseName, Qt::CaseInsensitive) < 0;
    });

    TreeNode root;
    root.text = QCoreApplication::translate("TypeCatalog", "Types");
    root.icon = QStringLiteral("folder");
    root.kind = TreeNode::Kind::Folder;
    root.expanded = true;
    for (const TypeInfo *type : std::as_const(roots)) {
        TreeNode node = typeNode(*type);
        node.expanded = true;
        node.children = subtypeNodes(type->nodeId, subtypes, shown);
        root.children.append(node);
    }
    return { root };
}

///
/// rief Lists the types a new node of a kind can be an instance of.
/// \param types Types keyed by node id, as types() returns them.
/// \param kind Folder, object or variable; methods have no type.
/// eturn The default type first (FolderType, BaseObjectType or BaseDataVariableType), then its concrete subtypes
///         sorted by name: those of imported models and, for variables, the standard data item types. Event types
///         and, for objects, folder types are left out.
///
QList<TypeInfo> instanceTypes(const QHash<QString, TypeInfo> &types, NodeDefinition::Kind kind)
{
    QString rootId;
    switch (kind) {
    case NodeDefinition::Kind::Folder:   rootId = kFolderType; break;
    case NodeDefinition::Kind::Object:   rootId = kBaseObjectType; break;
    case NodeDefinition::Kind::Variable: rootId = kBaseDataVariableType; break;
    case NodeDefinition::Kind::Method:   return {};
    }
    if (!types.contains(rootId))
        return {};

    QList<TypeInfo> subtypes;
    for (const TypeInfo &type : types) {
        if (type.isAbstract || type.nodeId == rootId || !derivesFrom(types, type.nodeId, rootId))
            continue;
        if (derivesFrom(types, type.nodeId, kBaseEventType)
            || (kind == NodeDefinition::Kind::Object && derivesFrom(types, type.nodeId, kFolderType)))
            continue;
        const bool standard = !type.nodeId.startsWith(QLatin1String("ns="));
        if (standard && !(kind == NodeDefinition::Kind::Variable && derivesFrom(types, type.nodeId, kDataItemType)))
            continue;
        subtypes.append(type);
    }
    std::sort(subtypes.begin(), subtypes.end(), [](const TypeInfo &a, const TypeInfo &b) {
        return a.browseName.compare(b.browseName, Qt::CaseInsensitive) < 0;
    });
    subtypes.prepend(types.value(rootId));
    return subtypes;
}

}
