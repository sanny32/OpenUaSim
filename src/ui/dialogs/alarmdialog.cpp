// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file alarmdialog.cpp
/// \brief Implements the dialog that edits the alarm of a variable.
///

#include <QPushButton>

#include <limits>

#include "alarmdialog.h"
#include "project/alarmrules.h"
#include "ui_alarmdialog.h"

namespace {

constexpr int kLimitDecimals = 3;

}

///
/// \brief Builds the dialog for a variable.
/// \param variable Variable whose alarm is edited; its data type selects limits or a normal value.
/// \param parent Parent widget.
///
AlarmDialog::AlarmDialog(const VariableInfo &variable, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AlarmDialog)
    , _dataType(variable.dataType)
    , _arrayLength(variable.arrayLength)
{
    ui->setupUi(this);
    ui->variableLabel->setText(tr("Alarm of '%1' (%2)").arg(variable.displayName, variable.dataType));

    const bool offNormal = AlarmRules::isOffNormal(variable.dataType);
    const QList<QWidget *> limitFields = { ui->highHighCheck, ui->highHighSpin, ui->highCheck, ui->highSpin,
                                           ui->lowCheck,      ui->lowSpin,      ui->lowLowCheck, ui->lowLowSpin };
    for (QWidget *field : limitFields)
        field->setVisible(!offNormal);
    ui->normalValueLabel->setVisible(offNormal);
    ui->normalValueCombo->setVisible(offNormal);
    for (QDoubleSpinBox *spin : { ui->highHighSpin, ui->highSpin, ui->lowSpin, ui->lowLowSpin }) {
        spin->setDecimals(kLimitDecimals);
        spin->setRange(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max());
    }

    const AlarmSettings &alarm = variable.alarm;
    ui->enabledCheck->setChecked(alarm.enabled);
    ui->highHighCheck->setChecked(alarm.highHighEnabled);
    ui->highHighSpin->setValue(alarm.highHigh);
    ui->highCheck->setChecked(alarm.highEnabled);
    ui->highSpin->setValue(alarm.high);
    ui->lowCheck->setChecked(alarm.lowEnabled);
    ui->lowSpin->setValue(alarm.low);
    ui->lowLowCheck->setChecked(alarm.lowLowEnabled);
    ui->lowLowSpin->setValue(alarm.lowLow);
    ui->normalValueCombo->setCurrentIndex(alarm.normalValue ? 1 : 0);
    ui->severitySpin->setValue(alarm.severity);

    connect(ui->enabledCheck, &QCheckBox::toggled, this, &AlarmDialog::validate);
    for (QCheckBox *check : { ui->highHighCheck, ui->highCheck, ui->lowCheck, ui->lowLowCheck })
        connect(check, &QCheckBox::toggled, this, &AlarmDialog::validate);
    for (QDoubleSpinBox *spin : { ui->highHighSpin, ui->highSpin, ui->lowSpin, ui->lowLowSpin })
        connect(spin, &QDoubleSpinBox::valueChanged, this, &AlarmDialog::validate);
    connect(ui->severitySpin, &QSpinBox::valueChanged, this, &AlarmDialog::validate);
    validate();
}

///
/// \brief Destroys the dialog.
///
AlarmDialog::~AlarmDialog()
{
    delete ui;
}

///
/// \brief Returns the edited alarm settings.
/// \return Settings; a disabled alarm keeps the entered values for later.
///
AlarmSettings AlarmDialog::alarm() const
{
    AlarmSettings alarm;
    alarm.enabled = ui->enabledCheck->isChecked();
    alarm.highHighEnabled = ui->highHighCheck->isChecked();
    alarm.highHigh = ui->highHighSpin->value();
    alarm.highEnabled = ui->highCheck->isChecked();
    alarm.high = ui->highSpin->value();
    alarm.lowEnabled = ui->lowCheck->isChecked();
    alarm.low = ui->lowSpin->value();
    alarm.lowLowEnabled = ui->lowLowCheck->isChecked();
    alarm.lowLow = ui->lowLowSpin->value();
    alarm.normalValue = ui->normalValueCombo->currentIndex() == 1;
    alarm.severity = ui->severitySpin->value();
    return alarm;
}

///
/// \brief Enables the settings with the alarm, shows why they cannot be applied, and allows OK only when they can.
///
void AlarmDialog::validate()
{
    ui->settingsHost->setEnabled(ui->enabledCheck->isChecked());
    const QString problem = AlarmRules::problem(alarm(), _dataType, _arrayLength);
    ui->problemLabel->setText(problem);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(problem.isEmpty());
}
