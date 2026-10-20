// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file alarmrules.cpp
/// \brief Implements the validation of variable alarms.
///

#include <QCoreApplication>
#include <QList>

#include <utility>

#include "alarmrules.h"

namespace {

///
/// \brief Translates a message of the alarm rules.
/// \param text Source text.
/// \return Translated text.
///
QString tr(const char *text)
{
    return QCoreApplication::translate("AlarmRules", text);
}

///
/// \brief Tells whether a data type holds numbers a limit alarm can compare.
/// \param dataType Built-in data type name.
/// \return False for Boolean, String and DateTime.
///
bool isNumeric(const QString &dataType)
{
    return dataType != QLatin1String("Boolean") && dataType != QLatin1String("String")
        && dataType != QLatin1String("DateTime");
}

}

namespace AlarmRules {

///
/// \brief Tells whether a variable of a data type gets an off-normal alarm instead of a limit alarm.
/// \param dataType Built-in data type name.
/// \return True for Boolean.
///
bool isOffNormal(const QString &dataType)
{
    return dataType == QLatin1String("Boolean");
}

///
/// \brief Returns why an alarm cannot be raised from a variable.
/// \param alarm Alarm settings; disabled settings are always accepted.
/// \param dataType Built-in data type name of the variable.
/// \param arrayLength Array length of the variable; 0 for a scalar.
/// \return Problem text, or empty when the alarm can be built: the variable is a numeric or Boolean scalar,
///         the severity is within 1..1000, and a numeric variable has at least one limit, with the
///         enabled limits ordered LowLow < Low < High < HighHigh.
///
QString problem(const AlarmSettings &alarm, const QString &dataType, int arrayLength)
{
    if (!alarm.enabled)
        return {};
    if (arrayLength > 0)
        return tr("An alarm needs a scalar variable");
    if (!isOffNormal(dataType) && !isNumeric(dataType))
        return tr("An alarm needs a numeric or Boolean variable");
    if (alarm.severity < 1 || alarm.severity > 1000)
        return tr("The severity must be between 1 and 1000");
    if (isOffNormal(dataType))
        return {};

    QList<double> limits;
    for (const auto &[enabled, limit] : { std::pair { alarm.lowLowEnabled, alarm.lowLow },
                                          std::pair { alarm.lowEnabled, alarm.low },
                                          std::pair { alarm.highEnabled, alarm.high },
                                          std::pair { alarm.highHighEnabled, alarm.highHigh } }) {
        if (enabled)
            limits.append(limit);
    }
    if (limits.isEmpty())
        return tr("Enable at least one limit");
    for (qsizetype index = 1; index < limits.size(); ++index) {
        if (limits.at(index - 1) >= limits.at(index))
            return tr("The limits must rise from LowLow through Low and High to HighHigh");
    }
    return {};
}

}
