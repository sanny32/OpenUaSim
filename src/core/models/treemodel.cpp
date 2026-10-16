// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file treemodel.cpp
/// \brief Implements the read-only navigation tree model.
///

#include "itemroles.h"
#include "treemodel.h"

///
/// \brief Storage node mirroring a TreeNode with a parent link.
///
struct TreeModel::Node
{
    TreeNode data;
    Node *parent = nullptr;
    int row = 0;
    std::vector<std::unique_ptr<Node>> children;
};

namespace {

///
/// \brief Copies a TreeNode subtree below a storage node.
/// \param source Subtree to copy; its children are copied, not the list itself.
/// \param target Storage node receiving the children.
/// \param hasTrailing Set to true when any copied node has trailing text.
///
template <typename Node>
void appendChildren(const QList<TreeNode> &source, Node *target, bool &hasTrailing)
{
    for (const TreeNode &child : source) {
        auto node = std::make_unique<Node>();
        node->data = child;
        node->data.children.clear();
        node->parent = target;
        node->row = static_cast<int>(target->children.size());
        hasTrailing = hasTrailing || !child.trailing.isEmpty();
        appendChildren(child.children, node.get(), hasTrailing);
        target->children.push_back(std::move(node));
    }
}

}

///
/// \brief Constructs an empty tree model.
/// \param parent Owning QObject.
///
TreeModel::TreeModel(QObject *parent)
    : QAbstractItemModel(parent)
    , _root(std::make_unique<Node>())
{
}

///
/// \brief Destroys the model and its node storage.
///
TreeModel::~TreeModel() = default;

///
/// \brief Replaces the whole tree, resetting attached views.
/// \param roots Top-level nodes with their descendants.
///
void TreeModel::setNodes(const QList<TreeNode> &roots)
{
    beginResetModel();
    _root = std::make_unique<Node>();
    _hasTrailing = false;
    appendChildren(roots, _root.get(), _hasTrailing);
    endResetModel();
}

///
/// \brief Finds the first node carrying an identifier.
/// \param id Identifier to look for.
/// \return Index of the node in column 0, or an invalid index when not found.
///
QModelIndex TreeModel::findById(const QString &id) const
{
    if (id.isEmpty())
        return {};
    return findIn(_root.get(), id);
}

///
/// \brief Searches a subtree depth-first for a node identifier.
/// \param parent Subtree root, itself not matched.
/// \param id Identifier to look for.
/// \return Index of the first match, or an invalid index.
///
QModelIndex TreeModel::findIn(const Node *parent, const QString &id) const
{
    for (const auto &child : parent->children) {
        if (child->data.id == id)
            return createIndex(child->row, 0, child.get());
        const QModelIndex found = findIn(child.get(), id);
        if (found.isValid())
            return found;
    }
    return {};
}

///
/// \brief Returns the index of a child node.
/// \param row Child row.
/// \param column Child column.
/// \param parent Parent index; invalid for top-level nodes.
/// \return Child index, or an invalid index when out of range.
///
QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return {};
    Node *parentNode = nodeFor(parent);
    return createIndex(row, column, parentNode->children.at(row).get());
}

///
/// \brief Returns the parent of a node.
/// \param child Node index.
/// \return Parent index, or an invalid index for top-level nodes.
///
QModelIndex TreeModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return {};
    Node *parentNode = nodeFor(child)->parent;
    if (!parentNode || parentNode == _root.get())
        return {};
    return createIndex(parentNode->row, 0, parentNode);
}

///
/// \brief Returns the number of children of a node.
/// \param parent Node index; invalid for the top level.
/// \return Child count, or 0 for second-column indexes.
///
int TreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;
    return static_cast<int>(nodeFor(parent)->children.size());
}

///
/// \brief Returns one column, or two when some node has trailing text.
/// \param parent Unused.
/// \return Column count.
///
int TreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return _hasTrailing ? 2 : 1;
}

///
/// \brief Returns node text, trailing text, and the custom item roles.
/// \param index Cell to query.
/// \param role Requested data role.
/// \return Value for the role, or an invalid variant.
///
QVariant TreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};
    const TreeNode &node = nodeFor(index)->data;

    if (index.column() == 1) {
        if (role == Qt::DisplayRole)
            return node.trailing;
        if (role == Qt::TextAlignmentRole)
            return QVariant::fromValue(Qt::AlignRight | Qt::AlignVCenter);
        return {};
    }

    switch (role) {
    case Qt::DisplayRole:
        return node.text;
    case ItemRoles::IconName:
        return node.icon.isEmpty() ? QVariant() : QVariant(node.icon);
    case ItemRoles::Id:
        return node.id;
    case ItemRoles::NodeKind:
        return static_cast<int>(node.kind);
    case ItemRoles::Expanded:
        return node.expanded;
    case ItemRoles::Current:
        return node.current;
    default:
        return {};
    }
}

///
/// \brief Resolves an index to its storage node.
/// \param index Index to resolve; invalid means the hidden root.
/// \return Storage node.
///
TreeModel::Node *TreeModel::nodeFor(const QModelIndex &index) const
{
    return index.isValid() ? static_cast<Node *>(index.internalPointer()) : _root.get();
}
