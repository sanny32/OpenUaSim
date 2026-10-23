// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file variablepropertieswidget.cpp
/// \brief Implements the editor of the settings and simulation of a variable.
///

#include <limits>

#include "data/projectconfig.h"
#include "formatting.h"
#include "simulation/signalgenerator.h"
#include "simulation/valuecast.h"
#include "ui_variablepropertieswidget.h"
#include "variablepropertieswidget.h"
#include "variablesmodel.h"
#include "viewsetup.h"

namespace {

constexpr int kMillisecondsPerSecond = 1000;
constexpr int kSecondsUnit = 1;

}

///
/// \brief Builds the editor.
/// \param parent Parent widget.
///
VariablePropertiesWidget::VariablePropertiesWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::VariablePropertiesWidget)
{
    ui->setupUi(this);
    fillCombos();
    ViewSetup::bindCopyButton(ui->copyNodeIdButton, ui->nodeIdEdit);
    ui->arrayLengthSpin->setMaximum(kMaxArrayLength);

    connect(ui->simulationCombo, &QComboBox::currentIndexChanged,
            this, &VariablePropertiesWidget::updateSimulationFields);
    connect(ui->applyButton, &QPushButton::clicked, this, &VariablePropertiesWidget::requestApply);
    connect(ui->alarmButton, &QPushButton::clicked, this, &VariablePropertiesWidget::requestAlarmEdit);
    connect(ui->revertButton, &QPushButton::clicked, this, &VariablePropertiesWidget::revert);
    updateSimulationFields();
    clear();
}

///
/// \brief Destroys the editor.
///
VariablePropertiesWidget::~VariablePropertiesWidget()
{
    delete ui;
}

///
/// \brief Shows a variable; a read-only one disables the editor and a model variable its structure.
/// \param variable Variable as stored.
///
void VariablePropertiesWidget::setVariable(const VariableInfo &variable)
{
    _variable = variable;
    ui->propertiesHost->setEnabled(!variable.readOnly);
    ui->propertiesActions->setEnabled(!variable.readOnly);
    const QList<QWidget *> structuralFields = { ui->displayNameEdit, ui->dataTypeCombo, ui->arrayLengthSpin };
    for (QWidget *widget : structuralFields)
        widget->setEnabled(!variable.fromModel);
    ui->nodeIdEdit->setText(variable.nodeId);
    ui->typeEdit->setText(variable.typeName);
    ui->displayNameEdit->setText(variable.displayName);
    ui->iconCombo->setCurrentIndex(ui->iconCombo->findData(variable.icon));
    const int dataTypeIndex = ui->dataTypeCombo->findText(variable.shownDataType());
    ui->dataTypeCombo->setCurrentIndex(dataTypeIndex);
    ui->dataTypeCombo->setPlaceholderText(dataTypeIndex < 0 ? variable.shownDataType() : QString());
    ui->arrayLengthSpin->setValue(variable.arrayLength);
    ui->accessCombo->setCurrentText(variable.access);
    ui->initialValueEdit->setText(Formatting::value(variable.initialValue));
    ui->qualityCombo->setCurrentIndex(ui->qualityCombo->findData(quint32(variable.configuredQuality)));
    ui->historizingCheck->setChecked(variable.historizing);
    ui->simulationCombo->setCurrentIndex(ui->simulationCombo->findData(int(variable.simulation.function)));
    ui->intervalSpin->setValue(variable.simulation.intervalMs);
    ui->intervalUnitCombo->setCurrentIndex(0);
    ui->periodSpin->setValue(variable.simulation.periodMs);
    ui->amplitudeSpin->setValue(variable.simulation.amplitude);
    ui->offsetSpin->setValue(variable.simulation.offset);
    ui->valuesEdit->setText(variable.simulation.values.join(QStringLiteral("; ")));
    updateSimulationFields();
}

///
/// \brief Empties and disables the editor.
///
void VariablePropertiesWidget::clear()
{
    _variable = VariableInfo();
    ui->nodeIdEdit->clear();
    ui->typeEdit->clear();
    ui->displayNameEdit->clear();
    ui->initialValueEdit->clear();
    ui->propertiesHost->setEnabled(false);
    ui->propertiesActions->setEnabled(false);
}

///
/// \brief Retranslates the editor when the language changes.
/// \param event Change event.
///
void VariablePropertiesWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        retranslate();
    QWidget::changeEvent(event);
}

///
/// \brief Lists the variable icons, the supported data types, simulation functions and qualities.
///
void VariablePropertiesWidget::fillCombos()
{
    ViewSetup::fillVariableIconCombo(ui->iconCombo);
    ui->dataTypeCombo->addItems(ValueCast::dataTypes());
    for (SimulationFunction function : SignalGenerator::functions())
        ui->simulationCombo->addItem(SignalGenerator::displayName(function), int(function));
    for (VariableInfo::Quality quality : VariableInfo::qualities())
        ui->qualityCombo->addItem(VariablesModel::qualityText(quality), quint32(quality));
}

///
/// \brief Reapplies the translated texts, including the combo boxes filled from code.
///
void VariablePropertiesWidget::retranslate()
{
    ui->retranslateUi(this);
    ViewSetup::retranslateVariableIconCombo(ui->iconCombo);
    for (int index = 0; index < ui->simulationCombo->count(); ++index)
        ui->simulationCombo->setItemText(
            index, SignalGenerator::displayName(SimulationFunction(ui->simulationCombo->itemData(index).toInt())));
    for (int index = 0; index < ui->qualityCombo->count(); ++index)
        ui->qualityCombo->setItemText(
            index, VariablesModel::qualityText(VariableInfo::Quality(ui->qualityCombo->itemData(index).toUInt())));
}

///
/// \brief Shows the value list for the List function and the waveform parameters for the others.
///
void VariablePropertiesWidget::updateSimulationFields()
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
/// \brief Asks to apply the edited settings of the shown variable.
///
void VariablePropertiesWidget::requestApply()
{
    if (!_variable.nodeId.isEmpty())
        emit applyRequested(editedVariable());
}

///
/// \brief Asks to edit the alarm of the shown variable as it is stored.
///
void VariablePropertiesWidget::requestAlarmEdit()
{
    if (!_variable.nodeId.isEmpty())
        emit alarmEditRequested(_variable);
}

///
/// \brief Discards edits by showing the stored variable again.
///
void VariablePropertiesWidget::revert()
{
    setVariable(_variable);
}

///
/// \brief Reads the editor.
/// \return Shown variable with the edited fields; an interval in seconds is converted to milliseconds,
///         an icon the picker does not offer is kept unless another one is chosen, and the initial
///         value is the typed text when it was edited and invalid otherwise.
///
VariableInfo VariablePropertiesWidget::editedVariable() const
{
    VariableInfo variable = _variable;
    variable.displayName = ui->displayNameEdit->text().trimmed();
    if (ui->iconCombo->currentIndex() >= 0)
        variable.icon = ui->iconCombo->currentData().toString();
    variable.dataType = ui->dataTypeCombo->currentText();
    variable.arrayLength = ui->arrayLengthSpin->value();
    variable.access = ui->accessCombo->currentText();
    variable.initialValue = ui->initialValueEdit->isModified() ? QVariant(ui->initialValueEdit->text()) : QVariant();
    variable.configuredQuality = VariableInfo::Quality(ui->qualityCombo->currentData().toUInt());
    variable.historizing = ui->historizingCheck->isChecked();
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
