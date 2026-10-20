// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file objectspage.cpp
/// \brief Implements the address space page.
///

#include <limits>

#include "data/projectconfig.h"
#include "formatting.h"
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
    ui->arrayLengthSpin->setMaximum(kMaxArrayLength);

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
    connect(ui->simulationCombo, &QComboBox::currentIndexChanged, this, &ObjectsPage::updateSimulationFields);
    connect(ui->applyButton, &QPushButton::clicked, this, &ObjectsPage::applyProperties);
    connect(ui->revertButton, &QPushButton::clicked, this, &ObjectsPage::revertProperties);
    updateSimulationFields();
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
/// \brief Lists the variables of the selected folder or object, or of the selected variable's or method's owner.
/// \param current Selected tree index.
///
void ObjectsPage::showTreeNode(const QModelIndex &current)
{
    const auto kind = static_cast<TreeNode::Kind>(current.data(ItemRoles::NodeKind).toInt());
    if (kind == TreeNode::Kind::Method) {
        const QModelIndex owner = current.parent();
        showObject(owner.data(ItemRoles::Id).toString(), owner.data().toString());
    } else if (kind == TreeNode::Kind::Variable) {
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
    _currentFromTable = false;
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
    _containerName = objectName;
    _variablesModel->setItems(_variables.value(objectId));
    showVariablesTitle();
    clearProperties();
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
/// \brief Reapplies the translated texts, including the combo boxes filled from code and the table headers.
///
void ObjectsPage::retranslate()
{
    ui->retranslateUi(this);
    showVariablesTitle();
    ViewSetup::retranslateVariableIconCombo(ui->iconCombo);
    for (int index = 0; index < ui->simulationCombo->count(); ++index)
        ui->simulationCombo->setItemText(
            index, SignalGenerator::displayName(SimulationFunction(ui->simulationCombo->itemData(index).toInt())));
    for (int index = 0; index < ui->qualityCombo->count(); ++index)
        ui->qualityCombo->setItemText(
            index, VariablesModel::qualityText(VariableInfo::Quality(ui->qualityCombo->itemData(index).toInt())));
    _variablesModel->retranslate();
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
    ui->propertiesHost->setEnabled(!variable.readOnly);
    ui->propertiesActions->setEnabled(!variable.readOnly);
    ui->nodeIdEdit->setText(variable.nodeId);
    ui->displayNameEdit->setText(variable.displayName);
    ui->iconCombo->setCurrentIndex(ui->iconCombo->findData(variable.icon));
    ui->dataTypeCombo->setCurrentText(variable.dataType);
    ui->arrayLengthSpin->setValue(variable.arrayLength);
    ui->accessCombo->setCurrentText(variable.access);
    ui->initialValueEdit->setText(Formatting::value(variable.initialValue));
    ui->qualityCombo->setCurrentIndex(ui->qualityCombo->findData(int(variable.configuredQuality)));
    ui->simulationCombo->setCurrentIndex(
        ui->simulationCombo->findData(int(variable.simulation.function)));
    ui->intervalSpin->setValue(variable.simulation.intervalMs);
    ui->intervalUnitCombo->setCurrentIndex(0);
    ui->periodSpin->setValue(variable.simulation.periodMs);
    ui->amplitudeSpin->setValue(variable.simulation.amplitude);
    ui->offsetSpin->setValue(variable.simulation.offset);
    ui->valuesEdit->setText(variable.simulation.values.join(QStringLiteral("; ")));
    updateSimulationFields();
    _currentNodeId = variable.nodeId;
    _currentFromTable = true;
    emit currentNodeChanged(_currentNodeId);
}

///
/// \brief Lists the variable icons, the supported data types and simulation functions.
///
void ObjectsPage::fillCombos()
{
    ViewSetup::fillVariableIconCombo(ui->iconCombo);
    ui->dataTypeCombo->addItems(ValueCast::dataTypes());
    for (SimulationFunction function : SignalGenerator::functions())
        ui->simulationCombo->addItem(SignalGenerator::displayName(function), int(function));
    for (auto quality : { VariableInfo::Quality::Good, VariableInfo::Quality::Uncertain, VariableInfo::Quality::Bad })
        ui->qualityCombo->addItem(VariablesModel::qualityText(quality), int(quality));
}

///
/// \brief Shows the value list for the List function and the waveform parameters for the others.
///
void ObjectsPage::updateSimulationFields()
{
    const bool list = SimulationFunction(ui->simulationCombo->currentData().toInt()) == SimulationFunction::List;
    const QList<QWidget *> waveformFields = { ui->amplitudeLabel, ui->amplitudeSpin, ui->offsetLabel,
                                              ui->offsetSpin, ui->periodLabel, ui->periodSpin };
    for (QWidget *widget : waveformFields)
        widget->setVisible(!list);
    ui->valuesLabel->setVisible(list);
    ui->valuesEdit->setVisible(list);
}

///
/// \brief Empties and disables the properties panel.
///
void ObjectsPage::clearProperties()
{
    ui->nodeIdEdit->clear();
    ui->displayNameEdit->clear();
    ui->initialValueEdit->clear();
    ui->propertiesHost->setEnabled(false);
    ui->propertiesActions->setEnabled(false);
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
/// \return Shown variable with the edited fields; an interval in seconds is converted to milliseconds,
///         an icon the picker does not offer is kept unless another one is chosen, and the initial
///         value is the typed text when it was edited and invalid otherwise.
///
VariableInfo ObjectsPage::editedVariable() const
{
    VariableInfo variable = _variablesModel->item(ui->variablesView->currentIndex().row());
    variable.displayName = ui->displayNameEdit->text().trimmed();
    if (ui->iconCombo->currentIndex() >= 0)
        variable.icon = ui->iconCombo->currentData().toString();
    variable.dataType = ui->dataTypeCombo->currentText();
    variable.arrayLength = ui->arrayLengthSpin->value();
    variable.access = ui->accessCombo->currentText();
    variable.initialValue = ui->initialValueEdit->isModified() ? QVariant(ui->initialValueEdit->text()) : QVariant();
    variable.configuredQuality = VariableInfo::Quality(ui->qualityCombo->currentData().toInt());
    variable.simulation.function = SimulationFunction(ui->simulationCombo->currentData().toInt());
    const int unit = ui->intervalUnitCombo->currentIndex() == kSecondsUnit ? kMillisecondsPerSecond : 1;
    variable.simulation.intervalMs = int(qMin<qint64>(qint64(ui->intervalSpin->value()) * unit,
                                                      std::numeric_limits<int>::max()));
    variable.simulation.periodMs = ui->periodSpin->value();
    variable.simulation.amplitude = ui->amplitudeSpin->value();
    variable.simulation.offset = ui->offsetSpin->value();
    variable.simulation.values.clear();
    const QStringList values = ui->valuesEdit->text().split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString &value : values) {
        if (!value.trimmed().isEmpty())
            variable.simulation.values.append(value.trimmed());
    }
    return variable;
}
