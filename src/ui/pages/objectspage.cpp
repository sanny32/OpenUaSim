// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file objectspage.cpp
/// \brief Implements the address space page.
///

#include <limits>

#include "itemroles.h"
#include "objectspage.h"
#include "simulation/signalgenerator.h"
#include "simulation/valuecast.h"
#include "treemodel.h"
#include "ui_objectspage.h"
#include "variablesmodel.h"
#include "viewsetup.h"

namespace {

constexpr int kMillisecondsPerSecond = 1000;
constexpr int kSecondsUnit = 1;

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
    fillCombos();
    ViewSetup::splitByStretch(ui->splitter);
    ViewSetup::bindCopyButton(ui->copyNodeIdButton, ui->nodeIdEdit);

    ViewSetup::installDelegate(ui->addressTree);
    ViewSetup::attachSearch(ui->addressTree, _treeModel, ui->searchEdit);
    ViewSetup::configureTable(ui->variablesView);
    ui->variablesView->setModel(_variablesModel);
    ViewSetup::fitColumnsWhenFilled(ui->variablesView);

    connect(ui->addressTree->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &ObjectsPage::showTreeNode);
    connect(ui->variablesView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &ObjectsPage::showVariable);
    connect(ui->applyButton, &QPushButton::clicked, this, &ObjectsPage::applyProperties);
    connect(ui->revertButton, &QPushButton::clicked, this, &ObjectsPage::revertProperties);
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
/// \brief Adds actions to the context menu of the address space tree.
/// \param actions Actions in menu order.
///
void ObjectsPage::setTreeActions(const QList<QAction *> &actions)
{
    ui->addressTree->addActions(actions);
    ui->addressTree->setContextMenuPolicy(Qt::ActionsContextMenu);
}

///
/// \brief Lists the variables of the selected folder or object, or of the selected variable's owner.
/// \param current Selected tree index.
///
void ObjectsPage::showTreeNode(const QModelIndex &current)
{
    const auto kind = static_cast<TreeNode::Kind>(current.data(ItemRoles::NodeKind).toInt());
    if (kind == TreeNode::Kind::Variable) {
        const QModelIndex owner = current.parent();
        showObject(owner.data(ItemRoles::Id).toString(), owner.data().toString());
        ViewSetup::selectRow(ui->variablesView,
                             _variablesModel->rowOf(current.data(ItemRoles::Id).toString()));
    } else if (kind == TreeNode::Kind::Object || kind == TreeNode::Kind::Folder) {
        const QString id = current.data(ItemRoles::Id).toString();
        showObject(id, _variables.contains(id) || kind == TreeNode::Kind::Object ? current.data().toString()
                                                                                : QString());
        ViewSetup::selectRow(ui->variablesView, 0);
    } else {
        showObject(QString(), QString());
    }
    // Selecting the first variable above made it current; the tree choice wins.
    _currentNodeId = current.data(ItemRoles::Id).toString();
    emit currentNodeChanged(_currentNodeId);
}

///
/// \brief Fills the variables table with the variables of a folder or object.
/// \param objectId Owner node id; empty clears the table.
/// \param objectName Owner display name for the panel title; empty shows the plain title.
///
void ObjectsPage::showObject(const QString &objectId, const QString &objectName)
{
    _containerId = objectId;
    _variablesModel->setItems(_variables.value(objectId));
    ui->variablesTitle->setText(objectName.isEmpty() ? tr("Variables")
                                                     : tr("Variables in '%1'").arg(objectName));
    clearProperties();
}

///
/// \brief Fills the properties panel from the selected variable.
/// \param current Selected table index.
///
void ObjectsPage::showVariable(const QModelIndex &current)
{
    if (!current.isValid()) {
        clearProperties();
        return;
    }
    const VariableInfo &variable = _variablesModel->item(current.row());
    ui->propertiesHost->setEnabled(true);
    ui->nodeIdEdit->setText(variable.nodeId);
    ui->displayNameEdit->setText(variable.displayName);
    ui->dataTypeCombo->setCurrentText(variable.dataType);
    ui->accessCombo->setCurrentText(variable.access);
    ui->simulationCombo->setCurrentIndex(
        ui->simulationCombo->findData(int(variable.simulation.function)));
    ui->intervalSpin->setValue(variable.simulation.intervalMs);
    ui->intervalUnitCombo->setCurrentIndex(0);
    ui->periodSpin->setValue(variable.simulation.periodMs);
    ui->amplitudeSpin->setValue(variable.simulation.amplitude);
    ui->offsetSpin->setValue(variable.simulation.offset);
    _currentNodeId = variable.nodeId;
    emit currentNodeChanged(_currentNodeId);
}

///
/// \brief Lists the supported data types and simulation functions.
///
void ObjectsPage::fillCombos()
{
    ui->dataTypeCombo->addItems(ValueCast::dataTypes());
    for (SimulationFunction function : SignalGenerator::functions())
        ui->simulationCombo->addItem(SignalGenerator::displayName(function), int(function));
}

///
/// \brief Empties and disables the properties panel.
///
void ObjectsPage::clearProperties()
{
    ui->nodeIdEdit->clear();
    ui->displayNameEdit->clear();
    ui->propertiesHost->setEnabled(false);
}

///
/// \brief Asks to apply the edited properties of the shown variable.
///
void ObjectsPage::applyProperties()
{
    if (ViewSetup::currentId(ui->variablesView).isEmpty())
        return;
    emit variableChangeRequested(editedVariable());
}

///
/// \brief Discards edits by showing the stored properties again.
///
void ObjectsPage::revertProperties()
{
    showVariable(ui->variablesView->currentIndex());
}

///
/// \brief Reads the properties panel.
/// \return Shown variable with the edited fields; an interval in seconds is converted to milliseconds.
///
VariableInfo ObjectsPage::editedVariable() const
{
    VariableInfo variable = _variablesModel->item(ui->variablesView->currentIndex().row());
    variable.displayName = ui->displayNameEdit->text().trimmed();
    variable.dataType = ui->dataTypeCombo->currentText();
    variable.access = ui->accessCombo->currentText();
    variable.simulation.function = SimulationFunction(ui->simulationCombo->currentData().toInt());
    const int unit = ui->intervalUnitCombo->currentIndex() == kSecondsUnit ? kMillisecondsPerSecond : 1;
    variable.simulation.intervalMs = int(qMin<qint64>(qint64(ui->intervalSpin->value()) * unit,
                                                      std::numeric_limits<int>::max()));
    variable.simulation.periodMs = ui->periodSpin->value();
    variable.simulation.amplitude = ui->amplitudeSpin->value();
    variable.simulation.offset = ui->offsetSpin->value();
    return variable;
}
