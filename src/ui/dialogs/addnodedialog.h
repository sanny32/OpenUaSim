// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file addnodedialog.h
/// \brief Declares the dialog that describes a new folder, object or variable.
///

#pragma once

#include <QDialog>
#include <QSet>
#include <QStringList>

#include "data/projectconfig.h"

namespace Ui {
class AddNodeDialog;
}

///
/// \brief Asks for the kind, browse name, node id and, for variables, data type, access and icon of a new node;
///        for methods, their action.
///
class AddNodeDialog : public QDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the dialog.
    /// \param parentName Display name of the node that receives the new node.
    /// \param siblingNames Browse names already used below that node.
    /// \param usedIds Node ids already used in the project.
    /// \param parent Parent widget.
    ///
    AddNodeDialog(const QString &parentName, const QStringList &siblingNames, const QSet<QString> &usedIds,
                  QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog.
    ///
    ~AddNodeDialog() override;

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
    void updateKindFields();
    void validate();

    Ui::AddNodeDialog *ui;
    QStringList _siblingNames;
    QSet<QString> _usedIds;
};
