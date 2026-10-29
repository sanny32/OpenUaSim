// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file methodpropertieswidget.cpp
/// \brief Implements the editor of a method.
///

#include <QEvent>
#include <QHeaderView>
#include "appicons.h"
#include "methodsourcedialog.h"
#include "project/portablenodeid.h"
#include "server/methodscenario.h"

#include "methodpropertieswidget.h"
#include "project/methodstatuses.h"
#include "ui_methodpropertieswidget.h"
#include "viewsetup.h"

namespace {

/// \brief Columns of the argument tables; the third one holds the description of an input or the value of an output.
enum Column { ColName, ColDataType, ColDescription, ColValue = ColDescription, ColSource };

/// \brief Names a source without duplicating its constant value column.
QString sourceName(const MethodValueSource &source)
{
    switch (source.kind) {
    case MethodValueSource::Kind::Constant: return MethodPropertiesWidget::tr("Constant");
    case MethodValueSource::Kind::Input:
        return source.inputIndex < 0 ? MethodPropertiesWidget::tr("Missing input")
                                    : MethodPropertiesWidget::tr("Input %1").arg(source.inputIndex + 1);
    case MethodValueSource::Kind::Variable: return source.variableId;
    case MethodValueSource::Kind::Simulation: return MethodPropertiesWidget::tr("Simulation");
    }
    return {};
}

/// \brief Builds a source cell with typed settings retained independently of its display text.
QTableWidgetItem *sourceItem(const MethodValueSource &source)
{
    auto *item = new QTableWidgetItem(sourceName(source));
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setData(Qt::UserRole, QVariant::fromValue(source));
    return item;
}

/// \brief Keeps references attached to their input when an earlier argument is removed.
void removeInputReference(MethodValueSource &source, int index)
{
    if (source.kind == MethodValueSource::Kind::Input && source.inputIndex >= index)
        source.inputIndex = source.inputIndex == index ? -1 : source.inputIndex - 1;
}

/// \brief Reads editable signature rows, preserving the scalar or array spelling used by the table.
QList<ArgumentInfo> argumentsFromTable(const QTableWidget *table, bool inputs)
{
    QList<ArgumentInfo> arguments;
    for (int row = 0; row < table->rowCount(); ++row) {
        ArgumentInfo argument;
        argument.name = table->item(row, ColName)->text().trimmed();
        argument.dataType = table->item(row, ColDataType)->text().trimmed();
        argument.array = argument.dataType.endsWith(QLatin1String("[]"));
        argument.valueType = argument.array ? argument.dataType.chopped(2) : argument.dataType;
        if (inputs)
            argument.description = table->item(row, ColDescription)->text();
        arguments.append(argument);
    }
    return arguments;
}

///
/// \brief Builds a table cell the user cannot edit.
/// \param text Cell text.
/// \return Cell without the editable flag.
///
QTableWidgetItem *readOnlyItem(const QString &text)
{
    auto *item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

}

///
/// \brief Builds the editor.
/// \param parent Parent widget.
///
MethodPropertiesWidget::MethodPropertiesWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MethodPropertiesWidget)
{
    ui->setupUi(this);
    for (QTableWidget *table : { ui->methodInputsTable, ui->methodOutputsTable, ui->methodSetActionsTable }) {
        table->horizontalHeader()->setMinimumSectionSize(24);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    }
    ui->methodAddInputButton->setIcon(AppIcons::icon(QStringLiteral("add-circle")));
    ui->methodRemoveInputButton->setIcon(AppIcons::icon(QStringLiteral("trash")));
    ui->methodAddOutputButton->setIcon(AppIcons::icon(QStringLiteral("add-circle")));
    ui->methodRemoveOutputButton->setIcon(AppIcons::icon(QStringLiteral("trash")));
    ui->methodAddActionButton->setIcon(AppIcons::icon(QStringLiteral("add-circle")));
    ui->methodRemoveActionButton->setIcon(AppIcons::icon(QStringLiteral("trash")));
    ViewSetup::bindCopyButton(ui->methodCopyNodeIdButton, ui->methodNodeIdEdit);
    for (quint32 code : MethodStatuses::codes())
        ui->methodStatusCombo->addItem(MethodStatuses::name(code), code);
    connect(ui->methodStatusCombo, &QComboBox::currentIndexChanged, this, &MethodPropertiesWidget::markEdited);
    connect(ui->methodOutputsTable, &QTableWidget::itemChanged, this, &MethodPropertiesWidget::markEdited);
    connect(ui->methodInputsTable, &QTableWidget::itemChanged, this, &MethodPropertiesWidget::markEdited);
    connect(ui->methodSetActionsTable, &QTableWidget::itemChanged, this, &MethodPropertiesWidget::markEdited);
    connect(ui->methodAddInputButton, &QAbstractButton::clicked, this, &MethodPropertiesWidget::addInput);
    connect(ui->methodRemoveInputButton, &QAbstractButton::clicked, this, &MethodPropertiesWidget::removeInput);
    connect(ui->methodAddOutputButton, &QAbstractButton::clicked, this, &MethodPropertiesWidget::addOutput);
    connect(ui->methodRemoveOutputButton, &QAbstractButton::clicked, this, &MethodPropertiesWidget::removeOutput);
    connect(ui->methodAddActionButton, &QAbstractButton::clicked, this, &MethodPropertiesWidget::addAction);
    connect(ui->methodRemoveActionButton, &QAbstractButton::clicked, this, &MethodPropertiesWidget::removeAction);
    connect(ui->methodActionUpButton, &QAbstractButton::clicked, this, &MethodPropertiesWidget::moveActionUp);
    connect(ui->methodActionDownButton, &QAbstractButton::clicked, this, &MethodPropertiesWidget::moveActionDown);
    connect(ui->methodApplyButton, &QPushButton::clicked, this, &MethodPropertiesWidget::requestApply);
    connect(ui->methodRevertButton, &QPushButton::clicked, this, &MethodPropertiesWidget::revert);
    connect(ui->methodOutputsTable, &QTableWidget::cellDoubleClicked, this, &MethodPropertiesWidget::editOutputSource);
    connect(ui->methodSetActionsTable, &QTableWidget::cellDoubleClicked, this, &MethodPropertiesWidget::editActionSource);
    clear();
}

///
/// \brief Destroys the editor.
///
MethodPropertiesWidget::~MethodPropertiesWidget()
{
    delete ui;
}

///
/// \brief Shows method scenario settings; only project scenarios have editable signatures.
/// \param method Method as described.
///
void MethodPropertiesWidget::setMethod(const MethodInfo &method)
{
    _filling = true;
    _method = method;
    const bool model = method.kind == MethodInfo::Kind::Model || signatureEditable();
    ui->methodNodeIdEdit->setText(method.nodeId);
    ui->methodNameEdit->setText(method.displayName);
    ui->methodDescriptionEdit->setText(method.description);
    ui->methodActionLabel->setVisible(method.kind == MethodInfo::Kind::Project);
    ui->methodActionEdit->setVisible(method.kind == MethodInfo::Kind::Project);
    showAction();
    ui->methodStatusLabel->setVisible(model);
    ui->methodStatusCombo->setVisible(model);
    ui->methodStatusCombo->setCurrentIndex(qMax(0, ui->methodStatusCombo->findData(method.response.statusCode)));
    fillInputs(method.inputs);
    fillOutputs(method.outputs, method.response.outputs);
    fillActions(method.response.actions);
    ui->methodInputButtons->setVisible(signatureEditable());
    ui->methodAddOutputButton->setVisible(signatureEditable());
    ui->methodRemoveOutputButton->setVisible(signatureEditable());
    ui->methodOutputButtons->setVisible(signatureEditable());
    ui->methodScenarioActions->setVisible(model);
    ui->methodInputsTable->setEditTriggers(signatureEditable() ? QAbstractItemView::AllEditTriggers : QAbstractItemView::NoEditTriggers);
    ui->methodInputsTable->setSelectionMode(signatureEditable() ? QAbstractItemView::SingleSelection : QAbstractItemView::NoSelection);
    ui->methodOutputsTable->setColumnHidden(ColValue, !model);
    ui->methodOutputsTable->setColumnHidden(ColSource, !model);
    ui->methodActions->setEnabled(model);
    ui->methodApplyButton->setEnabled(false);
    _filling = false;
}

///
/// \brief Empties and disables the editor.
///
void MethodPropertiesWidget::clear()
{
    setMethod(MethodInfo());
}

///
/// \brief Retranslates the editor when the language changes.
/// \param event Change event.
///
void MethodPropertiesWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        showAction();
    }
    QWidget::changeEvent(event);
}

///
/// \brief Lists the input arguments with their descriptions.
/// \param arguments Input arguments.
///
void MethodPropertiesWidget::fillInputs(const QList<ArgumentInfo> &arguments)
{
    ui->methodInputsTable->setRowCount(0);
    for (const ArgumentInfo &argument : arguments) {
        const int row = addArgumentRow(ui->methodInputsTable, argument);
        ui->methodInputsTable->setItem(row, ColDescription, readOnlyItem(argument.description));
        if (signatureEditable()) {
            for (int column = 0; column < 3; ++column)
                ui->methodInputsTable->item(row, column)->setFlags(ui->methodInputsTable->item(row, column)->flags() | Qt::ItemIsEditable);
        }
    }
}

///
/// \brief Lists the output arguments with their values, editable only for a model method.
/// \param arguments Output arguments.
/// \param values Value texts in argument order.
///
void MethodPropertiesWidget::fillOutputs(const QList<ArgumentInfo> &arguments, const QStringList &values)
{
    ui->methodOutputsTable->setRowCount(0);
    for (const ArgumentInfo &argument : arguments) {
        const int row = addArgumentRow(ui->methodOutputsTable, argument);
        ui->methodOutputsTable->setItem(row, ColValue, valueItem(argument, values.value(row)));
        MethodValueSource source = _method.response.sources.value(row);
        if (row >= _method.response.sources.size())
            source.value = values.value(row);
        ui->methodOutputsTable->setItem(row, ColSource, sourceItem(source));
        if (source.kind == MethodValueSource::Kind::Constant && row < _method.response.sources.size())
            ui->methodOutputsTable->item(row, ColValue)->setText(source.value);
        if (source.kind != MethodValueSource::Kind::Constant) {
            auto *value = ui->methodOutputsTable->item(row, ColValue);
            value->setText(QString());
            value->setFlags(value->flags() & ~Qt::ItemIsEditable);
        }
        if (signatureEditable()) {
            for (int column = 0; column < 2; ++column)
                ui->methodOutputsTable->item(row, column)->setFlags(ui->methodOutputsTable->item(row, column)->flags() | Qt::ItemIsEditable);
        }
    }
}

///
/// \brief Appends a row with the name and data type of an argument.
/// \param table Table receiving the row.
/// \param argument Argument to show; its description becomes the tooltip of the name.
/// \return Index of the new row.
///
int MethodPropertiesWidget::addArgumentRow(QTableWidget *table, const ArgumentInfo &argument)
{
    const int row = table->rowCount();
    table->insertRow(row);
    QTableWidgetItem *name = readOnlyItem(argument.name);
    name->setToolTip(argument.description);
    table->setItem(row, ColName, name);
    table->setItem(row, ColDataType, readOnlyItem(argument.dataType));
    return row;
}

///
/// \brief Builds the value cell of an output argument.
/// \param argument Output argument.
/// \param value Value text; empty for the default value.
/// \return Cell, editable for a model method when the value can be typed as text.
///
QTableWidgetItem *MethodPropertiesWidget::valueItem(const ArgumentInfo &argument, const QString &value) const
{
    const bool editable = (_method.kind == MethodInfo::Kind::Model || signatureEditable()) && !argument.valueType.isEmpty();
    QTableWidgetItem *item = editable ? new QTableWidgetItem(value) : readOnlyItem(value);
    if (argument.valueType.isEmpty())
        item->setToolTip(tr("The value of this data type cannot be typed; the default value is returned."));
    else if (argument.array)
        item->setToolTip(tr("Separate elements with semicolons; empty returns an empty array."));
    else
        item->setToolTip(tr("Empty returns the default value."));
    return item;
}

///
/// \brief Shows the action of a project method in the current language.
///
void MethodPropertiesWidget::showAction()
{
    if (signatureEditable()) {
        ui->methodActionEdit->setText(tr("Configured scenario"));
        return;
    }
    ui->methodActionEdit->setText(_method.action == NodeDefinition::MethodAction::Reset
                                      ? tr("Reset the variables next to it")
                                      : tr("Set a variable next to it from text"));
}

///
/// \brief Enables applying after the user changed the status or an output value.
///
void MethodPropertiesWidget::markEdited()
{
    if (!_filling)
        ui->methodApplyButton->setEnabled(true);
}

///
/// \brief Asks to apply the chosen status and the typed output values.
///
void MethodPropertiesWidget::requestApply()
{
    if (_method.nodeId.isEmpty())
        return;
    emit applyRequested(editedMethod());
}

///
/// \brief Discards the edits by showing the method again.
///
void MethodPropertiesWidget::revert()
{
    setMethod(_method);
}

///
/// \brief Supplies the address space variables offered in source selectors.
/// \param variables Available variables, keyed by their containing object.
///
void MethodPropertiesWidget::setVariables(const QHash<QString, QList<VariableInfo>> &variables)
{
    _variables.clear();
    for (const QList<VariableInfo> &group : variables)
        _variables.append(group);
}

/// \brief Restricts signature changes to project scenario methods.
bool MethodPropertiesWidget::signatureEditable() const
{
    return _method.kind == MethodInfo::Kind::Project && _method.action == NodeDefinition::MethodAction::Scenario;
}

/// \brief Collects the draft without changing the last applied method.
MethodInfo MethodPropertiesWidget::editedMethod() const
{
    MethodInfo edited = _method;
    if (signatureEditable()) {
        edited.inputs = argumentsFromTable(ui->methodInputsTable, true);
        edited.outputs = argumentsFromTable(ui->methodOutputsTable, false);
    }
    edited.response = {};
    edited.response.statusCode = ui->methodStatusCombo->currentData().toUInt();
    bool dynamic = false;
    for (int row = 0; row < ui->methodOutputsTable->rowCount(); ++row) {
        MethodValueSource source = ui->methodOutputsTable->item(row, ColSource)->data(Qt::UserRole).value<MethodValueSource>();
        source.value = ui->methodOutputsTable->item(row, ColValue)->text();
        dynamic |= source.kind != MethodValueSource::Kind::Constant;
        edited.response.outputs.append(source.kind == MethodValueSource::Kind::Constant ? source.value : QString());
        edited.response.sources.append(source);
    }
    if (!dynamic)
        edited.response.sources.clear();
    for (int row = 0; row < ui->methodSetActionsTable->rowCount(); ++row) {
        MethodSetAction action;
        action.enabled = ui->methodSetActionsTable->item(row, 0)->checkState() == Qt::Checked;
        action.targetId = ui->methodSetActionsTable->item(row, 1)->text().trimmed();
        if (!action.targetId.startsWith(QLatin1String("nsu=")) && !action.targetId.startsWith(QLatin1String("ns=1;")))
            action.targetId = MethodScenario::nodeReference(action.targetId, _method.namespaces);
        action.source = ui->methodSetActionsTable->item(row, 2)->data(Qt::UserRole).value<MethodValueSource>();
        edited.response.actions.append(action);
    }
    return edited;
}

/// \brief Restores assignments with enabled flags and source settings intact.
void MethodPropertiesWidget::fillActions(const QList<MethodSetAction> &actions)
{
    ui->methodSetActionsTable->setRowCount(0);
    for (const MethodSetAction &action : actions) {
        const int row = ui->methodSetActionsTable->rowCount();
        ui->methodSetActionsTable->insertRow(row);
        auto *enabled = readOnlyItem(tr("SET"));
        enabled->setFlags(enabled->flags() | Qt::ItemIsUserCheckable);
        enabled->setCheckState(action.enabled ? Qt::Checked : Qt::Unchecked);
        ui->methodSetActionsTable->setItem(row, 0, enabled);
        const QString resolved = PortableNodeId::toServer(action.targetId, _method.namespaces);
        ui->methodSetActionsTable->setItem(row, 1, new QTableWidgetItem(resolved.isEmpty() ? action.targetId : resolved));
        auto *source = sourceItem(action.source);
        if (action.source.kind == MethodValueSource::Kind::Constant)
            source->setText(tr("Constant: %1").arg(action.source.value));
        ui->methodSetActionsTable->setItem(row, 2, source);
    }
}

///
/// \brief Opens the output source editor only for a double-clicked source cell.
/// \param row Output argument row.
/// \param column Double-clicked column.
///
void MethodPropertiesWidget::editOutputSource(int row, int column)
{
    if (column != ColSource || row < 0
        || (_method.kind != MethodInfo::Kind::Model && !signatureEditable()))
        return;
    const MethodInfo edited = editedMethod();
    MethodValueSource source = ui->methodOutputsTable->item(row, ColSource)->data(Qt::UserRole).value<MethodValueSource>();
    source.value = ui->methodOutputsTable->item(row, ColValue)->text();
    MethodSourceDialog dialog(source, edited, _variables, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    source = dialog.source();
    ui->methodOutputsTable->setItem(row, ColSource, sourceItem(source));
    auto *value = valueItem(edited.outputs.at(row), source.kind == MethodValueSource::Kind::Constant ? source.value : QString());
    if (source.kind != MethodValueSource::Kind::Constant)
        value->setFlags(value->flags() & ~Qt::ItemIsEditable);
    ui->methodOutputsTable->setItem(row, ColValue, value);
    markEdited();
}

///
/// \brief Opens the action source editor only for a double-clicked source cell.
/// \param row Assignment row.
/// \param column Double-clicked column.
///
void MethodPropertiesWidget::editActionSource(int row, int column)
{
    if (column != 2 || row < 0
        || (_method.kind != MethodInfo::Kind::Model && !signatureEditable()))
        return;
    const MethodValueSource source = ui->methodSetActionsTable->item(row, 2)->data(Qt::UserRole).value<MethodValueSource>();
    MethodSourceDialog dialog(source, editedMethod(), _variables, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    auto actions = editedMethod().response.actions;
    actions[row].source = dialog.source();
    fillActions(actions);
    ui->methodSetActionsTable->selectRow(row);
    markEdited();
}

/// \brief Appends an editable input with a supported default type.
void MethodPropertiesWidget::addInput()
{
    const int row = ui->methodInputsTable->rowCount();
    ui->methodInputsTable->insertRow(row);
    ui->methodInputsTable->setItem(row, 0, new QTableWidgetItem(tr("Input%1").arg(row + 1)));
    ui->methodInputsTable->setItem(row, 1, new QTableWidgetItem(QStringLiteral("Double")));
    ui->methodInputsTable->setItem(row, 2, new QTableWidgetItem);
    markEdited();
}

/// \brief Removes the selected input; incompatible remaining references are rejected on apply.
void MethodPropertiesWidget::removeInput()
{
    const int row = ui->methodInputsTable->currentRow();
    if (row < 0)
        return;
    auto edited = editedMethod();
    for (MethodValueSource &source : edited.response.sources)
        removeInputReference(source, row);
    for (MethodSetAction &action : edited.response.actions)
        removeInputReference(action.source, row);
    ui->methodInputsTable->removeRow(row);
    for (qsizetype index = 0; index < edited.response.sources.size(); ++index)
        ui->methodOutputsTable->setItem(int(index), ColSource, sourceItem(edited.response.sources.at(index)));
    fillActions(edited.response.actions);
    markEdited();
}

/// \brief Appends an output with a default constant source.
void MethodPropertiesWidget::addOutput()
{
    const int row = ui->methodOutputsTable->rowCount();
    ui->methodOutputsTable->insertRow(row);
    ui->methodOutputsTable->setItem(row, 0, new QTableWidgetItem(tr("Output%1").arg(row + 1)));
    ui->methodOutputsTable->setItem(row, 1, new QTableWidgetItem(QStringLiteral("Double")));
    ui->methodOutputsTable->setItem(row, 2, new QTableWidgetItem);
    ui->methodOutputsTable->setItem(row, 3, sourceItem(MethodValueSource()));
    markEdited();
}

/// \brief Removes the selected output and its source.
void MethodPropertiesWidget::removeOutput()
{
    ui->methodOutputsTable->removeRow(ui->methodOutputsTable->currentRow());
    markEdited();
}

/// \brief Appends an assignment without applying its incomplete configuration.
void MethodPropertiesWidget::addAction()
{
    auto actions = editedMethod().response.actions;
    MethodSetAction action;
    if (!_variables.isEmpty())
        action.targetId = _variables.first().nodeId;
    actions.append(action);
    fillActions(actions);
    ui->methodSetActionsTable->selectRow(int(actions.size()) - 1);
    markEdited();
}

/// \brief Removes the selected assignment.
void MethodPropertiesWidget::removeAction()
{
    ui->methodSetActionsTable->removeRow(ui->methodSetActionsTable->currentRow());
    markEdited();
}

/// \brief Moves the selected assignment earlier in the sequence.
void MethodPropertiesWidget::moveActionUp() { moveAction(-1); }

/// \brief Moves the selected assignment later in the sequence.
void MethodPropertiesWidget::moveActionDown() { moveAction(1); }

/// \brief Reorders one assignment while retaining its source and enabled state.
void MethodPropertiesWidget::moveAction(int offset)
{
    const int row = ui->methodSetActionsTable->currentRow();
    auto actions = editedMethod().response.actions;
    if (row < 0 || row + offset < 0 || row + offset >= actions.size())
        return;
    actions.swapItemsAt(row, row + offset);
    fillActions(actions);
    ui->methodSetActionsTable->selectRow(row + offset);
    markEdited();
}
