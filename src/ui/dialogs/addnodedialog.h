// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file addnodedialog.h
/// \brief Declares the dialog that describes a new folder, object, variable or method.
///

#pragma once

#include "appbasedialog.h"
#include <QHash>
#include <QList>
#include <QSet>
#include <QString>

#include "data/projectconfig.h"
#include "data/typeinfo.h"

namespace Ui {
class AddNodeDialog;
}

///
/// \brief Asks for the parent, kind, type, browse name, node id and, for variables, data type, array length, access and icon
///        of a new node; for methods, their action.
///
class AddNodeDialog : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the dialog.
    /// \param nodes Project node tree whose folders and objects are offered as parents.
    /// \param parentId Folder or object selected first; empty or unknown selects the top level.
    /// \param parent Parent widget.
    ///
    AddNodeDialog(const QList<NodeDefinition> &nodes, const QString &parentId, QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog.
    ///
    ~AddNodeDialog() override;

    ///
    /// \brief Offers the types of the server for the new node.
    /// \param types Types keyed by server node id, as TypeCatalog::types() returns them.
    /// \param namespaces Namespace URIs of the server; the position of a URI is its index.
    ///
    void setTypes(const QHash<QString, TypeInfo> &types, const QStringList &namespaces);

    ///
    /// \brief Returns the chosen parent.
    /// \return Folder or object id; empty for the top level.
    ///
    QString parentId() const;

    ///
    /// \brief Returns the described node.
    /// \return Node without children; a variable starts with the default value of its type and no simulation.
    ///
    NodeDefinition node() const;

    ///
    /// \brief Returns why the entered node cannot be added.
    /// \return Problem text, or empty when the node can be added.
    ///
    QString problem() const;

    ///
    /// \brief Closes the dialog, or keeps it open and shows a warning when the entered node cannot be added.
    ///
    void accept() override;

private:
    void fillParentCombo(const QList<NodeDefinition> &nodes, const QString &path);
    void fillTypeCombo();
    void updateKindFields();
    void updateTypeFields();

    Ui::AddNodeDialog *ui;
    QList<NodeDefinition> _nodes;
    QSet<QString> _usedIds;
    QHash<QString, TypeInfo> _types;
    QStringList _namespaces;
};
