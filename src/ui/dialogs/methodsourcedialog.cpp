// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

/// \file methodsourcedialog.cpp
/// \brief Implements the editor for constant, input, variable and simulated method values.

#include "methodsourcedialog.h"
#include "server/methodscenario.h"
#include "project/portablenodeid.h"
#include "simulation/signalgenerator.h"
#include "ui_methodsourcedialog.h"

///
/// \brief Edits a source with the current method inputs and available variable references.
/// \param source Source to edit.
/// \param method Method supplying input names and namespace URIs.
/// \param variables Variables offered as sources.
/// \param parent Owning widget.
///
MethodSourceDialog::MethodSourceDialog(const MethodValueSource &source, const MethodInfo &method,
                                       const QList<VariableInfo> &variables, QWidget *parent)
    : AppBaseDialog(parent), ui(new Ui::MethodSourceDialog), _namespaces(method.namespaces)
{
    ui->setupUi(this);
    for (const ArgumentInfo &input : method.inputs)
        ui->inputCombo->addItem(input.name);
    ui->inputCombo->setCurrentIndex(source.inputIndex);
    for (const VariableInfo &variable : variables)
        ui->variableCombo->addItem(variable.nodeId, MethodScenario::nodeReference(variable.nodeId, _namespaces));
    const int selected = ui->variableCombo->findData(source.variableId);
    if (selected >= 0)
        ui->variableCombo->setCurrentIndex(selected);
    else
        ui->variableCombo->setEditText(source.variableId);
    for (SimulationFunction function : SignalGenerator::functions()) {
        if (function != SimulationFunction::None)
            ui->functionCombo->addItem(SignalGenerator::displayName(function), int(function));
    }
    ui->functionCombo->setCurrentIndex(qMax(0, ui->functionCombo->findData(int(source.simulation.function))));
    ui->constantEdit->setText(source.value);
    ui->intervalSpin->setValue(source.simulation.intervalMs);
    ui->periodSpin->setValue(source.simulation.periodMs);
    ui->amplitudeSpin->setValue(source.simulation.amplitude);
    ui->offsetSpin->setValue(source.simulation.offset);
    ui->valuesEdit->setText(source.simulation.values.join(QLatin1Char(';')));
    ui->sourceCombo->setCurrentIndex(int(source.kind));
    connect(ui->sourceCombo, &QComboBox::currentIndexChanged, this, &MethodSourceDialog::updateFields);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    updateFields();
}

/// \brief Releases the source editor.
MethodSourceDialog::~MethodSourceDialog() { delete ui; }

///
/// \brief Returns the configured source without applying it to the method.
/// \return Source using a persistent variable reference.
///
MethodValueSource MethodSourceDialog::source() const
{
    MethodValueSource result;
    result.kind = MethodValueSource::Kind(ui->sourceCombo->currentIndex());
    result.value = ui->constantEdit->text();
    result.inputIndex = ui->inputCombo->currentIndex();
    result.variableId = ui->variableCombo->currentData().toString();
    if (ui->variableCombo->currentText() != ui->variableCombo->itemText(ui->variableCombo->currentIndex())) {
        const QString text = ui->variableCombo->currentText().trimmed();
        result.variableId = text.startsWith(QLatin1String("nsu=")) ? text : MethodScenario::nodeReference(text, _namespaces);
    }
    result.simulation.function = SimulationFunction(ui->functionCombo->currentData().toInt());
    result.simulation.intervalMs = ui->intervalSpin->value();
    result.simulation.periodMs = ui->periodSpin->value();
    result.simulation.amplitude = ui->amplitudeSpin->value();
    result.simulation.offset = ui->offsetSpin->value();
    result.simulation.values = ui->valuesEdit->text().split(QLatin1Char(';'), Qt::SkipEmptyParts);
    return result;
}

/// \brief Enables only the fields used by the selected source kind.
void MethodSourceDialog::updateFields()
{
    const int kind = ui->sourceCombo->currentIndex();
    ui->constantEdit->setEnabled(kind == 0);
    ui->inputCombo->setEnabled(kind == 1);
    ui->variableCombo->setEnabled(kind == 2);
    ui->simulationGroup->setEnabled(kind == 3);
}
