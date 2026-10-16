// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file formatting.cpp
/// \brief Implements the text formatting shared by models and pages.
///

#include <QLocale>

#include "formatting.h"

namespace Formatting {

///
/// \brief Formats a timestamp to the second.
/// \param time Timestamp to format.
/// \return Text such as "2024-05-23 14:31:20".
///
QString dateTime(const QDateTime &time)
{
    return time.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
}

///
/// \brief Formats a timestamp to the millisecond.
/// \param time Timestamp to format.
/// \return Text such as "2024-05-23 14:31:20.004".
///
QString preciseDateTime(const QDateTime &time)
{
    return time.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
}

///
/// \brief Formats a duration as hours, minutes and seconds.
/// \param seconds Duration in seconds; negative values are treated as zero.
/// \return Text such as "00:27:36"; hours grow beyond two digits when needed.
///
QString duration(qint64 seconds)
{
    const qint64 total = qMax<qint64>(0, seconds);
    return QStringLiteral("%1:%2:%3")
        .arg(total / 3600, 2, 10, QLatin1Char('0'))
        .arg((total / 60) % 60, 2, 10, QLatin1Char('0'))
        .arg(total % 60, 2, 10, QLatin1Char('0'));
}

///
/// \brief Formats a variable value for display.
/// \param value Value to format.
/// \return Doubles with one decimal and group separators, booleans as "true"/"false",
///         anything else through QVariant::toString().
///
QString value(const QVariant &value)
{
    switch (value.typeId()) {
    case QMetaType::Double:
    case QMetaType::Float:
        return decimal(value.toDouble());
    case QMetaType::Bool:
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    default:
        return value.toString();
    }
}

///
/// \brief Formats a floating-point number with one decimal and group separators.
/// \param number Number to format.
/// \return Locale-formatted text.
///
QString decimal(double number)
{
    return QLocale().toString(number, 'f', 1);
}

}
