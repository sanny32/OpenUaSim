// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file valuecast.cpp
/// \brief Implements the conversion of generated numbers to OPC UA built-in data types.
///

#include <QDateTime>
#include <QTimeZone>
#include <QtMath>

#include <cmath>
#include <limits>

#include "valuecast.h"

namespace {

///
/// \brief Rounds a number and clamps it to the range of an integer type.
/// \param value Number to convert; NaN becomes zero.
/// \return Nearest representable integer.
///
template<typename T>
T saturate(double value)
{
    if (std::isnan(value))
        return T(0);
    const double rounded = std::round(value);
    if (rounded <= double(std::numeric_limits<T>::min()))
        return std::numeric_limits<T>::min();
    // The double nearest to max() of a 64-bit type rounds up past it, so compare with >=.
    if (rounded >= double(std::numeric_limits<T>::max()))
        return std::numeric_limits<T>::max();
    return T(rounded);
}

///
/// \brief Tells whether a value holds a number or a boolean.
/// \param value Value to inspect.
/// \return True for bool, integer and floating-point types.
///
bool isNumeric(const QVariant &value)
{
    switch (value.typeId()) {
    case QMetaType::Bool:
    case QMetaType::Char:
    case QMetaType::SChar:
    case QMetaType::UChar:
    case QMetaType::Short:
    case QMetaType::UShort:
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::Long:
    case QMetaType::ULong:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Float:
    case QMetaType::Double:
        return true;
    default:
        return false;
    }
}

}

namespace ValueCast {

///
/// \brief Returns the built-in data types a simulated variable can have.
/// \return Type names in display order, e.g. "Boolean", "Int32", "Double".
///
QStringList dataTypes()
{
    return { QStringLiteral("Boolean"), QStringLiteral("SByte"),  QStringLiteral("Byte"),
             QStringLiteral("Int16"),   QStringLiteral("UInt16"), QStringLiteral("Int32"),
             QStringLiteral("UInt32"),  QStringLiteral("Int64"),  QStringLiteral("UInt64"),
             QStringLiteral("Float"),   QStringLiteral("Double"), QStringLiteral("String"),
             QStringLiteral("DateTime") };
}

///
/// \brief Tells whether a data type name is supported.
/// \param dataType Built-in data type name.
/// \return True when dataTypes() lists it.
///
bool isSupported(const QString &dataType)
{
    return dataTypes().contains(dataType);
}

///
/// \brief Converts a generated number to a data type.
/// \param value Generated number.
/// \param dataType Built-in data type name.
/// \return Value of the matching Qt type; integers are rounded and saturated, Boolean is true
///         when the value rounds to non-zero, DateTime counts seconds since the Unix epoch in UTC.
///         Invalid for an unsupported type.
///
QVariant fromDouble(double value, const QString &dataType)
{
    if (dataType == QLatin1String("Boolean"))
        return QVariant(saturate<qint64>(value) != 0);
    if (dataType == QLatin1String("SByte"))
        return QVariant::fromValue(saturate<qint8>(value));
    if (dataType == QLatin1String("Byte"))
        return QVariant::fromValue(saturate<quint8>(value));
    if (dataType == QLatin1String("Int16"))
        return QVariant::fromValue(saturate<qint16>(value));
    if (dataType == QLatin1String("UInt16"))
        return QVariant::fromValue(saturate<quint16>(value));
    if (dataType == QLatin1String("Int32"))
        return QVariant::fromValue(saturate<qint32>(value));
    if (dataType == QLatin1String("UInt32"))
        return QVariant::fromValue(saturate<quint32>(value));
    if (dataType == QLatin1String("Int64"))
        return QVariant::fromValue(saturate<qint64>(value));
    if (dataType == QLatin1String("UInt64"))
        return QVariant::fromValue(saturate<quint64>(value));
    if (dataType == QLatin1String("Float"))
        return QVariant::fromValue(float(value));
    if (dataType == QLatin1String("Double"))
        return QVariant::fromValue(value);
    if (dataType == QLatin1String("String"))
        return QString::number(value, 'g', 6);
    if (dataType == QLatin1String("DateTime"))
        return QDateTime::fromSecsSinceEpoch(saturate<qint64>(value), QTimeZone::UTC);
    return {};
}

///
/// \brief Returns the value a variable of a data type starts with.
/// \param dataType Built-in data type name.
/// \return Zero, false, an empty string or the Unix epoch; invalid for an unsupported type.
///
QVariant defaultValue(const QString &dataType)
{
    if (dataType == QLatin1String("String"))
        return QString();
    return fromDouble(0.0, dataType);
}

///
/// \brief Returns the Qt type that holds values of a data type.
/// \param dataType Built-in data type name.
/// \return Type such as QMetaType::Int for "Int32"; UnknownType for an unsupported type.
///
QMetaType::Type metaType(const QString &dataType)
{
    const QVariant value = defaultValue(dataType);
    return value.isValid() ? QMetaType::Type(value.typeId()) : QMetaType::UnknownType;
}

///
/// \brief Converts any value to a data type.
/// \param value Value to convert; numbers and booleans follow fromDouble(), other values the
///        QVariant conversion.
/// \param dataType Built-in data type name.
/// \return Converted value, or defaultValue() when the value does not convert.
///
QVariant convert(const QVariant &value, const QString &dataType)
{
    if (isNumeric(value))
        return fromDouble(value.toDouble(), dataType);
    QVariant converted = value;
    if (converted.isValid() && converted.convert(QMetaType(metaType(dataType))))
        return converted;
    return defaultValue(dataType);
}

}
