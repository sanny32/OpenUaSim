// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file objectspage.h
/// \brief Declares the address space page.
///

#pragma once

#include <QHash>
#include <QList>
#include <QWidget>

#include "data/treenode.h"
#include "data/variableinfo.h"

namespace Ui {
class ObjectsPage;
}

class QItemSelection;
class TreeModel;
class VariablesModel;

///
/// \brief Page browsing the address space, the variables of an object and their simulation.
///
class ObjectsPage : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the page.
    /// \param parent Parent widget.
    ///
    explicit ObjectsPage(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the page.
    ///
    ~ObjectsPage() override;

    ///
    /// \brief Shows an address space.
    /// \param addressSpace Address space tree.
    /// \param variables Variables of each object, keyed by the object node id.
    ///
    void setAddressSpace(const QList<TreeNode> &addressSpace,
                         const QHash<QString, QList<VariableInfo>> &variables);

    ///
    /// \brief Returns the variables table model.
    /// \return Model of the variables of the shown object.
    ///
    const VariablesModel *variablesModel() const;

private:
    void fillCombos();
    void showTreeNode(const QModelIndex &current);
    void showObject(const QString &objectId, const QString &objectName);
    void showVariable(const QModelIndex &current);
    void clearProperties();

    Ui::ObjectsPage *ui;
    TreeModel *_treeModel;
    VariablesModel *_variablesModel;
    QHash<QString, QList<VariableInfo>> _variables;
};
