// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file alarmrules.h
/// \brief Declares the validation of variable alarms.
///

#pragma once

#include <QString>

#include "data/variableinfo.h"

namespace AlarmRules {

///
/// \brief Tells whether a variable of a data type gets an off-normal alarm instead of a limit alarm.
/// \param dataType Built-in data type name.
/// \return True for Boolean.
///
bool isOffNormal(const QString &dataType);

///
/// \brief Returns why an alarm cannot be raised from a variable.
/// \param alarm Alarm settings; disabled settings are always accepted.
/// \param dataType Built-in data type name of the variable.
/// \param arrayLength Array length of the variable; 0 for a scalar.
/// \return Problem text, or empty when the alarm can be built: the variable is a numeric or Boolean scalar,
///         the severity is within 1..1000, and a numeric variable has at least one limit, with the
///         enabled limits ordered LowLow < Low < High < HighHigh.
///
QString problem(const AlarmSettings &alarm, const QString &dataType, int arrayLength);

}
