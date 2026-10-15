// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file formatting.h
/// \brief Declares the text formatting shared by models and pages.
///

#pragma once

#include <QDateTime>
#include <QString>
#include <QVariant>

namespace Formatting {

///
/// \brief Formats a timestamp to the second.
/// \param time Timestamp to format.
/// \return Text such as "2024-05-23 14:31:20".
///
QString dateTime(const QDateTime &time);

///
/// \brief Formats a timestamp to the millisecond.
/// \param time Timestamp to format.
/// \return Text such as "2024-05-23 14:31:20.004".
///
QString preciseDateTime(const QDateTime &time);

///
/// \brief Formats a duration as hours, minutes and seconds.
/// \param seconds Duration in seconds; negative values are treated as zero.
/// \return Text such as "00:27:36"; hours grow beyond two digits when needed.
///
QString duration(qint64 seconds);

///
/// \brief Formats a variable value for display.
/// \param value Value to format.
/// \return Doubles with one decimal and group separators, booleans as "true"/"false",
///         anything else through QVariant::toString().
///
QString value(const QVariant &value);

///
/// \brief Formats a floating-point number with one decimal and group separators.
/// \param number Number to format.
/// \return Locale-formatted text.
///
QString decimal(double number);

}
