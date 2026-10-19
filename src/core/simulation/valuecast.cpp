// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file valuecast.cpp
/// \brief Implements the conversion of generated numbers to OPC UA built-in data types.
///

#include <QDateTime>
#include <QLocale>
#include <QTimeZone>
#include <QtMath>

#include <cmath>
#include <limits>
#include <type_traits>

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
/// \brief Parses a number written in the C or the system locale.
/// \param text Text to parse.
/// \param number Receives the number; qlonglong, qulonglong or double.
/// \return False when neither locale reads the whole text as a number.
///
template<typename T>
bool parseNumber(const QString &text, T &number)
{
    for (const QLocale &locale : { QLocale::c(), QLocale() }) {
        bool ok = false;
        if constexpr (std::is_same_v<T, double>)
            number = locale.toDouble(text, &ok);
        else if constexpr (std::is_signed_v<T>)
            number = locale.toLongLong(text, &ok);
        else
            number = locale.toULongLong(text, &ok);
        if (ok)
            return true;
    }
    return false;
}

///
/// \brief Parses an integer of a fixed-size type.
/// \param text Text to parse.
/// \return Value of type T, or invalid when the text is no integer or out of range.
///
template<typename T>
QVariant integerFromText(const QString &text)
{
    using Wide = std::conditional_t<std::is_signed_v<T>, qlonglong, qulonglong>;
    Wide number = 0;
    if (!parseNumber(text, number) || number < Wide(std::numeric_limits<T>::min())
        || number > Wide(std::numeric_limits<T>::max()))
        return {};
    return QVariant::fromValue(T(number));
}

///
/// \brief Parses a Boolean written as true/false or 1/0.
/// \param text Text to parse; case is ignored.
/// \return Boolean value, or invalid for other text.
///
QVariant booleanFromText(const QString &text)
{
    if (text.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0 || text == QLatin1String("1"))
        return true;
    if (text.compare(QLatin1String("false"), Qt::CaseInsensitive) == 0 || text == QLatin1String("0"))
        return false;
    return {};
}

///
/// \brief Parses a floating-point number.
/// \param text Text to parse.
/// \param single Whether the value must fit a Float.
/// \return Float or Double value, or invalid when the text is no number or too large for a Float.
///
QVariant realFromText(const QString &text, bool single)
{
    double number = 0.0;
    if (!parseNumber(text, number))
        return {};
    if (!single)
        return number;
    if (std::isfinite(number) && std::abs(number) > double(std::numeric_limits<float>::max()))
        return {};
    return QVariant::fromValue(float(number));
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

///
/// \brief Tells whether a value holds a sequence of values, such as a QVariantList or a QList<double>.
/// \param value Value to inspect.
/// \return True for sequences; false for strings and byte arrays, which Qt can also iterate.
///
bool isArray(const QVariant &value)
{
    return value.typeId() != QMetaType::QString && value.typeId() != QMetaType::QByteArray
        && value.canConvert<QVariantList>();
}

///
/// \brief Converts a value to a fixed-length array of a data type.
/// \param value A list, whose elements are converted, or a scalar, which fills every element.
/// \param dataType Built-in data type name of the elements.
/// \param length Number of elements.
/// \return List of exactly length elements; a short list is padded with defaultValue().
///
QVariantList toArray(const QVariant &value, const QString &dataType, int length)
{
    QVariantList result;
    result.reserve(qMax(0, length));
    if (isArray(value)) {
        const QVariantList elements = value.toList();
        for (int index = 0; index < length; ++index)
            result.append(index < elements.size() ? convert(elements.at(index), dataType) : defaultValue(dataType));
        return result;
    }
    const QVariant element = convert(value, dataType);
    for (int index = 0; index < length; ++index)
        result.append(element);
    return result;
}

///
/// \brief Parses text a user typed as a value of a data type.
/// \param text Text to parse; surrounding spaces are ignored except for String.
/// \param dataType Built-in data type name.
/// \return Value of the matching Qt type, or invalid when the text is not a value of the type:
///         Boolean takes true/false or 1/0, numbers the C or the system locale and must fit the
///         type, DateTime takes ISO 8601.
///
QVariant fromText(const QString &text, const QString &dataType)
{
    if (dataType == QLatin1String("String"))
        return text;
    const QString trimmed = text.trimmed();
    if (dataType == QLatin1String("Boolean"))
        return booleanFromText(trimmed);
    if (dataType == QLatin1String("SByte"))
        return integerFromText<qint8>(trimmed);
    if (dataType == QLatin1String("Byte"))
        return integerFromText<quint8>(trimmed);
    if (dataType == QLatin1String("Int16"))
        return integerFromText<qint16>(trimmed);
    if (dataType == QLatin1String("UInt16"))
        return integerFromText<quint16>(trimmed);
    if (dataType == QLatin1String("Int32"))
        return integerFromText<qint32>(trimmed);
    if (dataType == QLatin1String("UInt32"))
        return integerFromText<quint32>(trimmed);
    if (dataType == QLatin1String("Int64"))
        return integerFromText<qint64>(trimmed);
    if (dataType == QLatin1String("UInt64"))
        return integerFromText<quint64>(trimmed);
    if (dataType == QLatin1String("Float"))
        return realFromText(trimmed, true);
    if (dataType == QLatin1String("Double"))
        return realFromText(trimmed, false);
    if (dataType == QLatin1String("DateTime")) {
        const QDateTime time = QDateTime::fromString(trimmed, Qt::ISODateWithMs);
        return time.isValid() ? QVariant(time) : QVariant();
    }
    return {};
}

}
