// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file addnodedialog.h
/// \brief Declares the dialog that describes a new folder, object or variable.
///

#pragma once

#include <QDialog>
#include <QList>
#include <QSet>
#include <QString>

#include "data/projectconfig.h"

namespace Ui {
class AddNodeDialog;
}

///
/// \brief Asks for the parent, kind, browse name, node id and, for variables, data type, access and icon of
///        a new node; for methods, their action.
///
class AddNodeDialog : public QDialog
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
    /// \return Problem text, or empty when OK may be pressed.
    ///
    QString problem() const;

private:
    void fillParentCombo(const QList<NodeDefinition> &nodes, const QString &path);
    void updateKindFields();
    void validate();

    Ui::AddNodeDialog *ui;
    QList<NodeDefinition> _nodes;
    QSet<QString> _usedIds;
};
