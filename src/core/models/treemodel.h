// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file treemodel.h
/// \brief Declares the read-only navigation tree model.
///

#pragma once

#include <memory>
#include <vector>

#include <QAbstractItemModel>

#include "data/treenode.h"

///
/// \brief Read-only item model presenting a TreeNode hierarchy.
///
/// The model has a second, right-aligned column only when some node carries trailing text.
///
class TreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs an empty tree model.
    /// \param parent Owning QObject.
    ///
    explicit TreeModel(QObject *parent = nullptr);

    ///
    /// \brief Destroys the model and its node storage.
    ///
    ~TreeModel() override;

    ///
    /// \brief Replaces the whole tree, resetting attached views.
    /// \param roots Top-level nodes with their descendants.
    ///
    void setNodes(const QList<TreeNode> &roots);

    ///
    /// \brief Finds the first node carrying an identifier.
    /// \param id Identifier to look for.
    /// \return Index of the node in column 0, or an invalid index when not found.
    ///
    QModelIndex findById(const QString &id) const;

    ///
    /// \brief Replaces the icon of the first node carrying an identifier.
    /// \param id Identifier of the node.
    /// \param icon Icon resource name.
    /// \return False when no node carries the identifier.
    ///
    bool setIcon(const QString &id, const QString &icon);

    ///
    /// \brief Returns the index of a child node.
    /// \param row Child row.
    /// \param column Child column.
    /// \param parent Parent index; invalid for top-level nodes.
    /// \return Child index, or an invalid index when out of range.
    ///
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns the parent of a node.
    /// \param child Node index.
    /// \return Parent index, or an invalid index for top-level nodes.
    ///
    QModelIndex parent(const QModelIndex &child) const override;

    ///
    /// \brief Returns the number of children of a node.
    /// \param parent Node index; invalid for the top level.
    /// \return Child count, or 0 for second-column indexes.
    ///
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns one column, or two when some node has trailing text.
    /// \param parent Unused.
    /// \return Column count.
    ///
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns node text, trailing text, and the custom item roles.
    /// \param index Cell to query.
    /// \param role Requested data role.
    /// \return Value for the role, or an invalid variant.
    ///
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    struct Node;

    Node *nodeFor(const QModelIndex &index) const;
    QModelIndex findIn(const Node *parent, const QString &id) const;

    std::unique_ptr<Node> _root;
    bool _hasTrailing = false;
};
