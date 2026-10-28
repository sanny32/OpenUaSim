// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file alarmdialog.cpp
/// \brief Implements the dialog that edits the alarm of a variable.
///

#include <QPushButton>

#include <limits>
#include <utility>

#include "alarmdialog.h"
#include "project/alarmrules.h"
#include "ui_alarmdialog.h"

namespace {

constexpr int kRealDecimals = 3;

///
/// \brief Returns the lowest and highest value of a numeric type as doubles.
///
template <typename T>
std::pair<double, double> rangeOf()
{
    return { double(std::numeric_limits<T>::lowest()), double(std::numeric_limits<T>::max()) };
}

///
/// \brief Returns the values a limit can take for a data type.
/// \param dataType Built-in data type name of the variable.
/// \return Lowest and highest value of the type; the whole double range for Double or an unknown type.
///
std::pair<double, double> limitRange(const QString &dataType)
{
    if (dataType == QLatin1String("SByte"))
        return rangeOf<qint8>();
    if (dataType == QLatin1String("Byte"))
        return rangeOf<quint8>();
    if (dataType == QLatin1String("Int16"))
        return rangeOf<qint16>();
    if (dataType == QLatin1String("UInt16"))
        return rangeOf<quint16>();
    if (dataType == QLatin1String("Int32"))
        return rangeOf<qint32>();
    if (dataType == QLatin1String("UInt32"))
        return rangeOf<quint32>();
    if (dataType == QLatin1String("Int64"))
        return rangeOf<qint64>();
    if (dataType == QLatin1String("UInt64"))
        return rangeOf<quint64>();
    if (dataType == QLatin1String("Float"))
        return rangeOf<float>();
    return rangeOf<double>();
}

///
/// \brief Returns how many decimals a limit of a data type shows.
/// \param dataType Built-in data type name of the variable.
/// \return None for the integer types, a few for Float and Double.
///
int limitDecimals(const QString &dataType)
{
    const bool real = dataType == QLatin1String("Float") || dataType == QLatin1String("Double");
    return real ? kRealDecimals : 0;
}

}

///
/// \brief Builds the dialog for a variable.
/// \param variable Variable whose alarm is edited; its data type selects limits or a normal value.
/// \param parent Parent widget.
///
AlarmDialog::AlarmDialog(const VariableInfo &variable, QWidget *parent)
    : AppBaseDialog(parent)
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
    const auto [lowest, highest] = limitRange(variable.dataType);
    for (QDoubleSpinBox *spin : { ui->highHighSpin, ui->highSpin, ui->lowSpin, ui->lowLowSpin }) {
        spin->setDecimals(limitDecimals(variable.dataType));
        spin->setRange(lowest, highest);
    }

    const AlarmSettings &alarm = variable.alarm;
    ui->alarmGroup->setChecked(alarm.enabled);
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

    connect(ui->alarmGroup, &QGroupBox::toggled, this, &AlarmDialog::validate);
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
    alarm.enabled = ui->alarmGroup->isChecked();
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
/// \brief Enables each limit with its check box, shows why the settings cannot be applied, and allows OK only when they can.
///
void AlarmDialog::validate()
{
    ui->highHighSpin->setEnabled(ui->highHighCheck->isChecked());
    ui->highSpin->setEnabled(ui->highCheck->isChecked());
    ui->lowSpin->setEnabled(ui->lowCheck->isChecked());
    ui->lowLowSpin->setEnabled(ui->lowLowCheck->isChecked());
    const QString problem = AlarmRules::problem(alarm(), _dataType, _arrayLength);
    ui->problemLabel->setText(problem);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(problem.isEmpty());
}
