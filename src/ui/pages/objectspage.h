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

class QAction;
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
    /// \brief Shows new variable values without rebuilding the tree.
    /// \param values New values keyed by node id.
    ///
    void updateValues(const QHash<QString, QVariant> &values);

    ///
    /// \brief Returns the variables table model.
    /// \return Model of the variables of the shown object.
    ///
    const VariablesModel *variablesModel() const;

    ///
    /// \brief Shows changed settings of one variable without rebuilding the tree.
    /// \param variable Updated description.
    ///
    void updateVariable(const VariableInfo &variable);

    ///
    /// \brief Selects a node of the address space tree.
    /// \param nodeId Node id.
    /// \return False when the tree has no such node.
    ///
    bool selectNode(const QString &nodeId);

    ///
    /// \brief Returns the node the tree selection or the variables table points at.
    /// \return Node id of the tree node or variable row the user selected last; empty when none.
    ///
    QString currentNodeId() const;

    ///
    /// \brief Returns the node shown in the variables table, i.e. the owner of the variables.
    /// \return Node id of the folder or object whose variables are listed; empty when none.
    ///
    QString currentContainerId() const;

    ///
    /// \brief Returns the nodes selected in the view the current node was picked in.
    /// \return Node ids of the selected tree nodes or variable rows; the current node when nothing is selected.
    ///
    QStringList selectedNodeIds() const;

    ///
    /// \brief Adds actions to the context menu of the address space tree.
    /// \param actions Actions in menu order.
    ///
    void setTreeActions(const QList<QAction *> &actions);

signals:
    ///
    /// \brief Asks to apply edited properties of the shown variable, or the typed value of the shown property.
    /// \param variable Variable with the edited display name, icon, data type, access and simulation, or a property
    ///        with its typed value text as initial value.
    ///
    void variableChangeRequested(const VariableInfo &variable);

    ///
    /// \brief Asks to edit the alarm of the shown variable.
    /// \param variable Variable as stored, with its current alarm settings.
    ///
    void alarmEditRequested(const VariableInfo &variable);

    ///
    /// \brief Announces that the current node changed.
    /// \param nodeId Node id now returned by currentNodeId().
    ///
    void currentNodeChanged(const QString &nodeId);

protected:
    void changeEvent(QEvent *event) override;

private:
    void showVariablesTitle();
    void retranslate();
    void showTreeNode(const QModelIndex &current);
    void showContainer(const QString &containerId);
    void setNestedVariablesShown(bool shown);
    void showVariable(const QModelIndex &current);
    void clearProperties();
    void selectVariableRow(int row);

    Ui::ObjectsPage *ui;
    TreeModel *_treeModel;
    VariablesModel *_variablesModel;
    QList<TreeNode> _addressSpace;
    QHash<QString, QList<VariableInfo>> _variables;
    QString _containerId;
    QString _containerName;
    QString _currentNodeId;
    bool _currentFromTable = false;
};
