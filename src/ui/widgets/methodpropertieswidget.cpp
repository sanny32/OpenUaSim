// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file methodpropertieswidget.cpp
/// \brief Implements the editor of a method.
///

#include <QEvent>

#include "methodpropertieswidget.h"
#include "project/methodstatuses.h"
#include "ui_methodpropertieswidget.h"
#include "viewsetup.h"

namespace {

/// \brief Columns of the argument tables; the third one holds the description of an input or the value of an output.
enum Column { ColName, ColDataType, ColDescription, ColValue = ColDescription };

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
    ViewSetup::bindCopyButton(ui->methodCopyNodeIdButton, ui->methodNodeIdEdit);
    for (quint32 code : MethodStatuses::codes())
        ui->methodStatusCombo->addItem(MethodStatuses::name(code), code);
    connect(ui->methodStatusCombo, &QComboBox::currentIndexChanged, this, &MethodPropertiesWidget::markEdited);
    connect(ui->methodOutputsTable, &QTableWidget::itemChanged, this, &MethodPropertiesWidget::markEdited);
    connect(ui->methodApplyButton, &QPushButton::clicked, this, &MethodPropertiesWidget::requestApply);
    connect(ui->methodRevertButton, &QPushButton::clicked, this, &MethodPropertiesWidget::revert);
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
/// \brief Shows a method; the response status and output values are editable for a method of an imported model.
/// \param method Method as described.
///
void MethodPropertiesWidget::setMethod(const MethodInfo &method)
{
    _filling = true;
    _method = method;
    const bool model = method.kind == MethodInfo::Kind::Model;
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
    ui->methodOutputsTable->setColumnHidden(ColValue, !model);
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
    const bool editable = _method.kind == MethodInfo::Kind::Model && !argument.valueType.isEmpty();
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
    MethodInfo edited = _method;
    edited.response.statusCode = ui->methodStatusCombo->currentData().toUInt();
    edited.response.outputs.clear();
    for (int row = 0; row < ui->methodOutputsTable->rowCount(); ++row)
        edited.response.outputs.append(ui->methodOutputsTable->item(row, ColValue)->text());
    emit applyRequested(edited);
}

///
/// \brief Discards the edits by showing the method again.
///
void MethodPropertiesWidget::revert()
{
    setMethod(_method);
}
