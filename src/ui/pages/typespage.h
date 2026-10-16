// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file typespage.h
/// \brief Declares the type system page.
///

#pragma once

#include <QHash>
#include <QList>
#include <QWidget>

#include "data/treenode.h"
#include "data/typeinfo.h"

namespace Ui {
class TypesPage;
}

class ReferencesModel;
class SubtypesModel;
class TreeModel;

///
/// \brief Page browsing the type hierarchy, the subtypes of a type and its properties.
///
class TypesPage : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the page.
    /// \param parent Parent widget.
    ///
    explicit TypesPage(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the page.
    ///
    ~TypesPage() override;

    ///
    /// \brief Shows a type system.
    /// \param typeTree Type hierarchy tree.
    /// \param types Types keyed by node id.
    ///
    void setTypes(const QList<TreeNode> &typeTree, const QHash<QString, TypeInfo> &types);

    ///
    /// \brief Returns the subtypes table model.
    /// \return Model of the subtypes of the selected type.
    ///
    const SubtypesModel *subtypesModel() const;

private:
    void showTreeNode(const QModelIndex &current);
    void showSubtype(const QModelIndex &current);
    void showProperties(const QString &typeId);
    QList<TypeInfo> subtypesOf(const QString &typeId) const;

    Ui::TypesPage *ui;
    TreeModel *_treeModel;
    SubtypesModel *_subtypesModel;
    ReferencesModel *_referencesModel;
    QHash<QString, TypeInfo> _types;
};
