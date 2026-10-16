// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file valuecast.h
/// \brief Declares the conversion of generated numbers to OPC UA built-in data types.
///

#pragma once

#include <QStringList>
#include <QVariant>

namespace ValueCast {

///
/// \brief Returns the built-in data types a simulated variable can have.
/// \return Type names in display order, e.g. "Boolean", "Int32", "Double".
///
QStringList dataTypes();

///
/// \brief Tells whether a data type name is supported.
/// \param dataType Built-in data type name.
/// \return True when dataTypes() lists it.
///
bool isSupported(const QString &dataType);

///
/// \brief Converts a generated number to a data type.
/// \param value Generated number.
/// \param dataType Built-in data type name.
/// \return Value of the matching Qt type; integers are rounded and saturated, Boolean is true
///         when the value rounds to non-zero, DateTime counts seconds since the Unix epoch in UTC.
///         Invalid for an unsupported type.
///
QVariant fromDouble(double value, const QString &dataType);

///
/// \brief Returns the value a variable of a data type starts with.
/// \param dataType Built-in data type name.
/// \return Zero, false, an empty string or the Unix epoch; invalid for an unsupported type.
///
QVariant defaultValue(const QString &dataType);

}
