// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file objectspage.cpp
/// \brief Implements the address space page.
///

#include "itemroles.h"
#include "objectspage.h"
#include "simulation/signalgenerator.h"
#include "simulation/valuecast.h"
#include "treemodel.h"
#include "ui_objectspage.h"
#include "variablesmodel.h"
#include "viewsetup.h"

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
}

///
/// \brief Destroys the page.
///
ObjectsPage::~ObjectsPage()
{
    delete ui;
}

///
/// \brief Shows an address space.
/// \param addressSpace Address space tree.
/// \param variables Variables of each object, keyed by the object node id.
///
void ObjectsPage::setAddressSpace(const QList<TreeNode> &addressSpace,
                                  const QHash<QString, QList<VariableInfo>> &variables)
{
    _variables = variables;
    _treeModel->setNodes(addressSpace);
    ViewSetup::applyInitialState(ui->addressTree);
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
/// \brief Lists the variables of the selected object, or of the selected variable's owner.
/// \param current Selected tree index.
///
void ObjectsPage::showTreeNode(const QModelIndex &current)
{
    const auto kind = static_cast<TreeNode::Kind>(current.data(ItemRoles::NodeKind).toInt());
    if (kind == TreeNode::Kind::Object) {
        showObject(current.data(ItemRoles::Id).toString(), current.data().toString());
        ViewSetup::selectRow(ui->variablesView, 0);
    } else if (kind == TreeNode::Kind::Variable) {
        const QModelIndex object = current.parent();
        showObject(object.data(ItemRoles::Id).toString(), object.data().toString());
        ViewSetup::selectRow(ui->variablesView,
                             _variablesModel->rowOf(current.data(ItemRoles::Id).toString()));
    } else {
        showObject(QString(), QString());
    }
}

///
/// \brief Fills the variables table with the variables of an object.
/// \param objectId Object node id; empty clears the table.
/// \param objectName Object display name for the panel title.
///
void ObjectsPage::showObject(const QString &objectId, const QString &objectName)
{
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
    ui->amplitudeSpin->setValue(variable.simulation.amplitude);
    ui->offsetSpin->setValue(variable.simulation.offset);
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
