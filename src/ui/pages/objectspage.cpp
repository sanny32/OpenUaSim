// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file objectspage.cpp
/// \brief Implements the address space page.
///

#include <QSettings>

#include "appicons.h"
#include "itemroles.h"
#include "objectspage.h"
#include "propertynodewidget.h"
#include "treemodel.h"
#include "ui_objectspage.h"
#include "variablelisting.h"
#include "variablepropertieswidget.h"
#include "variablesmodel.h"
#include "viewsetup.h"

namespace {

const char *const kNestedVariablesKey = "objects/nestedVariables";

}

///
/// \brief Builds the page.
/// \param parent Parent widget.
///
ObjectsPage::ObjectsPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ObjectsPage)
    , _treeModel(new TreeModel(this))
    , _variablesModel(new VariablesModel(this))
{
    ui->setupUi(this);
    ViewSetup::splitByStretch(ui->splitter);
    ui->nestedVariablesButton->setIcon(AppIcons::icon(QStringLiteral("list-tree")));
    ui->nestedVariablesButton->setChecked(QSettings().value(kNestedVariablesKey, true).toBool());

    ViewSetup::installDelegate(ui->addressTree);
    ViewSetup::attachSearch(ui->addressTree, _treeModel, ui->searchEdit);
    ViewSetup::configureTable(ui->variablesView);
    ui->variablesView->setModel(_variablesModel);
    ViewSetup::fitColumnsWhenFilled(ui->variablesView);
    ViewSetup::growColumnWithContents(ui->variablesView, VariablesModel::ColValue);

    connect(ui->addressTree->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &ObjectsPage::showTreeNode);
    connect(ui->variablesView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &ObjectsPage::showVariable);
    connect(ui->nestedVariablesButton, &QToolButton::toggled, this, &ObjectsPage::setNestedVariablesShown);
    connect(ui->variableProperties, &VariablePropertiesWidget::applyRequested,
            this, &ObjectsPage::variableChangeRequested);
    connect(ui->variableProperties, &VariablePropertiesWidget::alarmEditRequested,
            this, &ObjectsPage::alarmEditRequested);
    connect(ui->propertyNodeProperties, &PropertyNodeWidget::applyRequested,
            this, &ObjectsPage::variableChangeRequested);
}

///
/// \brief Destroys the page.
///
ObjectsPage::~ObjectsPage()
{
    delete ui;
}

///
/// \brief Shows an address space, keeping the selected node when it still exists.
/// \param addressSpace Address space tree.
/// \param variables Variables of each object, keyed by the object node id.
///
void ObjectsPage::setAddressSpace(const QList<TreeNode> &addressSpace,
                                  const QHash<QString, QList<VariableInfo>> &variables)
{
    const QString previousTreeId = ViewSetup::currentId(ui->addressTree);
    const QString previousVariableId = ViewSetup::currentId(ui->variablesView);

    _addressSpace = addressSpace;
    _variables = variables;
    _treeModel->setNodes(addressSpace);
    ViewSetup::applyInitialState(ui->addressTree);
    if (!previousTreeId.isEmpty() && ViewSetup::selectById(ui->addressTree, previousTreeId)
        && !previousVariableId.isEmpty()) {
        ViewSetup::selectRow(ui->variablesView, _variablesModel->rowOf(previousVariableId));
    }
}

///
/// \brief Shows new variable values without rebuilding the tree.
/// \param values New values keyed by node id.
///
void ObjectsPage::updateValues(const QHash<QString, QVariant> &values)
{
    for (QList<VariableInfo> &variables : _variables) {
        for (VariableInfo &variable : variables) {
            const auto value = values.constFind(variable.nodeId);
            if (value != values.constEnd())
                variable.value = value.value();
        }
    }
    _variablesModel->updateValues(values);
}

///
/// \brief Returns the variables table model.
/// \return Model of the variables of the shown object.
///
const VariablesModel *ObjectsPage::variablesModel() const
{
    return _variablesModel;
}

///
/// \brief Shows changed settings of one variable without rebuilding the tree.
/// \param variable Updated description.
///
void ObjectsPage::updateVariable(const VariableInfo &variable)
{
    for (QList<VariableInfo> &variables : _variables) {
        for (VariableInfo &stored : variables) {
            if (stored.nodeId == variable.nodeId)
                stored = variable;
        }
    }
    _treeModel->setIcon(variable.nodeId, variable.icon);
    const int row = _variablesModel->rowOf(variable.nodeId);
    if (row < 0)
        return;
    _variablesModel->setItem(row, variable);
    if (ui->variablesView->currentIndex().row() == row)
        showVariable(ui->variablesView->currentIndex());
}

///
/// \brief Selects a node of the address space tree.
/// \param nodeId Node id.
/// \return False when the tree has no such node.
///
bool ObjectsPage::selectNode(const QString &nodeId)
{
    return ViewSetup::selectById(ui->addressTree, nodeId);
}

///
/// \brief Returns the node the tree selection or the variables table points at.
/// \return Node id of the tree node or variable row the user selected last; empty when none.
///
QString ObjectsPage::currentNodeId() const
{
    return _currentNodeId;
}

///
/// \brief Returns the node shown in the variables table, i.e. the owner of the variables.
/// \return Node id of the folder or object whose variables are listed; empty when none.
///
QString ObjectsPage::currentContainerId() const
{
    return _containerId;
}

///
/// \brief Returns the nodes selected in the view the current node was picked in.
/// \return Node ids of the selected tree nodes or variable rows; the current node when nothing is selected.
///
QStringList ObjectsPage::selectedNodeIds() const
{
    const QAbstractItemView *view = _currentFromTable ? static_cast<const QAbstractItemView *>(ui->variablesView)
                                                      : ui->addressTree;
    QStringList ids;
    const QModelIndexList rows = view->selectionModel()->selectedRows();
    for (const QModelIndex &row : rows)
        ids.append(row.data(ItemRoles::Id).toString());
    if (ids.isEmpty() && !_currentNodeId.isEmpty())
        ids.append(_currentNodeId);
    return ids;
}

///
/// \brief Adds actions to the context menu of the address space tree.
/// \param actions Actions in menu order.
///
void ObjectsPage::setTreeActions(const QList<QAction *> &actions)
{
    ui->addressTree->addActions(actions);
    ui->addressTree->setContextMenuPolicy(Qt::ActionsContextMenu);
}

///
/// \brief Lists the variables of the selected folder or object, or of the selected variable's, property's or
///        method's owner.
/// \param current Selected tree index.
///
void ObjectsPage::showTreeNode(const QModelIndex &current)
{
    const auto kind = static_cast<TreeNode::Kind>(current.data(ItemRoles::NodeKind).toInt());
    if (kind == TreeNode::Kind::Method) {
        showContainer(current.parent().data(ItemRoles::Id).toString());
    } else if (kind == TreeNode::Kind::Variable || kind == TreeNode::Kind::Property) {
        showContainer(current.parent().data(ItemRoles::Id).toString());
        ViewSetup::selectRow(ui->variablesView,
                             _variablesModel->rowOf(current.data(ItemRoles::Id).toString()));
    } else if (kind == TreeNode::Kind::Object || kind == TreeNode::Kind::Folder) {
        showContainer(current.data(ItemRoles::Id).toString());
        ViewSetup::selectRow(ui->variablesView, 0);
    } else {
        showContainer(QString());
    }
    // Selecting the first variable above made it current; the tree choice wins.
    _currentNodeId = current.data(ItemRoles::Id).toString();
    _currentFromTable = false;
    emit currentNodeChanged(_currentNodeId);
}

///
/// \brief Fills the variables table with the variables of a folder or object, and of the nodes
///        below it while nested variables are shown.
/// \param containerId Folder or object node id; empty or unknown clears the table.
///
void ObjectsPage::showContainer(const QString &containerId)
{
    const TreeNode *container = VariableListing::find(_addressSpace, containerId);
    const VariableListing::Listing listing =
        container ? VariableListing::collect(*container, _variables, ui->nestedVariablesButton->isChecked())
                  : VariableListing::Listing();
    _containerId = containerId;
    // A folder without variables, such as Server, keeps the plain title.
    const bool titled = container && (container->kind == TreeNode::Kind::Object || !listing.variables.isEmpty());
    _containerName = titled ? container->text : QString();
    _variablesModel->setVariables(listing.variables, listing.locations);
    showVariablesTitle();
    clearProperties();
}

///
/// \brief Lists the shown container again with or without its nested variables and remembers the choice.
/// \param shown Whether the variables of nested folders and objects are listed.
///
void ObjectsPage::setNestedVariablesShown(bool shown)
{
    QSettings().setValue(kNestedVariablesKey, shown);
    const bool currentFromTable = _currentFromTable;
    const QString variableId = ViewSetup::currentId(ui->variablesView);
    showContainer(_containerId);
    const int row = _variablesModel->rowOf(variableId);
    ViewSetup::selectRow(ui->variablesView, qMax(row, 0));
    if (!currentFromTable || row < 0) {
        _currentNodeId = ViewSetup::currentId(ui->addressTree);
        _currentFromTable = false;
        emit currentNodeChanged(_currentNodeId);
    }
}

///
/// \brief Titles the variables panel after the shown folder or object.
///
void ObjectsPage::showVariablesTitle()
{
    ui->variablesTitle->setText(_containerName.isEmpty() ? tr("Variables")
                                                         : tr("Variables in '%1'").arg(_containerName));
}

///
/// \brief Retranslates the page when the language changes.
/// \param event Change event.
///
void ObjectsPage::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        retranslate();
    QWidget::changeEvent(event);
}

///
/// \brief Reapplies the translated texts, including the table headers.
///
void ObjectsPage::retranslate()
{
    ui->retranslateUi(this);
    showVariablesTitle();
    _variablesModel->retranslate();
}

///
/// \brief Shows the selected variable or property in the editor of its kind.
/// \param current Selected table index.
///
void ObjectsPage::showVariable(const QModelIndex &current)
{
    if (!current.isValid()) {
        clearProperties();
        return;
    }
    const VariableInfo &variable = _variablesModel->item(current.row());
    if (variable.property) {
        ui->propertyNodeProperties->setNode(variable);
        ui->propertiesStack->setCurrentWidget(ui->propertyNodeProperties);
    } else {
        ui->variableProperties->setVariable(variable);
        ui->propertiesStack->setCurrentWidget(ui->variableProperties);
    }
    _currentNodeId = variable.nodeId;
    _currentFromTable = true;
    emit currentNodeChanged(_currentNodeId);
}

///
/// \brief Empties and disables the properties panel.
///
void ObjectsPage::clearProperties()
{
    ui->variableProperties->clear();
    ui->propertyNodeProperties->clear();
    ui->propertiesStack->setCurrentWidget(ui->variableProperties);
}
