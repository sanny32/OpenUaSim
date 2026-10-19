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

///
/// \brief Returns the Qt type that holds values of a data type.
/// \param dataType Built-in data type name.
/// \return Type such as QMetaType::Int for "Int32"; UnknownType for an unsupported type.
///
QMetaType::Type metaType(const QString &dataType);

///
/// \brief Converts any value to a data type.
/// \param value Value to convert; numbers and booleans follow fromDouble(), other values the
///        QVariant conversion.
/// \param dataType Built-in data type name.
/// \return Converted value, or defaultValue() when the value does not convert.
///
QVariant convert(const QVariant &value, const QString &dataType);

///
/// \brief Parses text a user typed as a value of a data type.
/// \param text Text to parse; surrounding spaces are ignored except for String.
/// \param dataType Built-in data type name.
/// \return Value of the matching Qt type, or invalid when the text is not a value of the type:
///         Boolean takes true/false or 1/0, numbers the C or the system locale and must fit the
///         type, DateTime takes ISO 8601.
///
QVariant fromText(const QString &text, const QString &dataType);

}
