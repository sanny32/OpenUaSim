// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file variableproperties.cpp
/// \brief Implements the properties of variables, such as the EURange of an analog item, and their text form.
///

#include <QCoreApplication>
#include <QRegularExpression>

#include <QUaProperty>
#include <QUaServer>

#include "addressspace.h"
#include "importednodes.h"
#include "modelvariables.h"
#include "simulation/valuecast.h"
#include "typecatalog.h"
#include "variableproperties.h"

namespace {

const QString kArraySuffix = QStringLiteral("[]");
const QString kRange = QStringLiteral("Range");
const QString kEUInformation = QStringLiteral("EUInformation");
const QString kLocalizedText = QStringLiteral("LocalizedText");
const QString kEnumValueType = QStringLiteral("EnumValueType");
const QString kUnitsNamespace = QStringLiteral("http://www.opcfoundation.org/UA/units/un/cefact");
constexpr qint32 kUnknownUnitId = -1;

///
/// \brief Property that a data item type of namespace 0 declares (OPC UA Part 8, 5.3).
///
struct Declaration
{
    /// \brief Type declaring the property, which its subtypes inherit.
    QUaNodeId typeId;
    /// \brief Browse name in namespace 0.
    QString browseName;
    /// \brief Data type name as PropertyInfo::dataType holds it.
    QString dataType;
};

///
/// \brief Lists the properties the data item types declare; the mandatory ones are among them, so describe() shows
///        them in this order whether instantiating the type added them or not.
/// \return Declarations of BaseAnalogType, TwoStateDiscreteType, MultiStateDiscreteType,
///         MultiStateValueDiscreteType and DataItemType.
///
const QList<Declaration> &declarations()
{
    static const QList<Declaration> list {
        { QUaNodeId(0, 15318u), QStringLiteral("EURange"), kRange },
        { QUaNodeId(0, 15318u), QStringLiteral("EngineeringUnits"), kEUInformation },
        { QUaNodeId(0, 15318u), QStringLiteral("InstrumentRange"), kRange },
        { QUaNodeId(0, 2373u), QStringLiteral("TrueState"), kLocalizedText },
        { QUaNodeId(0, 2373u), QStringLiteral("FalseState"), kLocalizedText },
        { QUaNodeId(0, 2376u), QStringLiteral("EnumStrings"), kLocalizedText + kArraySuffix },
        { QUaNodeId(0, 11238u), QStringLiteral("EnumValues"), kEnumValueType + kArraySuffix },
        { QUaNodeId(0, 11238u), QStringLiteral("ValueAsText"), kLocalizedText },
        { QUaNodeId(0, 2365u), QStringLiteral("Definition"), QStringLiteral("String") },
        { QUaNodeId(0, 2365u), QStringLiteral("ValuePrecision"), QStringLiteral("Double") },
    };
    return list;
}

///
/// \brief Finds the data item declaration of a property that the type of a variable inherits.
/// \param server Server holding the variable.
/// \param variable Served variable.
/// \param browseName Property browse name.
/// \return Declaration, or null when the type declares no such data item property.
///
const Declaration *declarationOf(const QUaServer &server, const QUaBaseDataVariable &variable,
                                 const QString &browseName)
{
    const QUaNodeId typeId = variable.typeDefinitionNodeId();
    for (const Declaration &declaration : declarations()) {
        if (declaration.browseName == browseName && server.isSubtypeOf(typeId, declaration.typeId))
            return &declaration;
    }
    return nullptr;
}

///
/// \brief Finds a property of a variable by its browse name in any namespace.
/// \param variable Served variable.
/// \param browseName Browse name without namespace.
/// \return Property, or null.
///
QUaProperty *findProperty(const QUaBaseDataVariable &variable, const QString &browseName)
{
    const QList<QUaNode *> children = variable.browseChildren();
    for (QUaNode *child : children) {
        if (auto *property = qobject_cast<QUaProperty *>(child); property && child->browseName().name() == browseName)
            return property;
    }
    return nullptr;
}

///
/// \brief Returns the node id of a new property, derived from its variable like the children a type declares.
/// \param variable Variable receiving the property.
/// \param browseName Property browse name.
/// \return Node id such as "ns=1;s=Tank.Level.EngineeringUnits", or null to let the server assign one to the
///         property of a variable without a string node id in the application namespace.
///
QUaNodeId propertyNodeId(const QUaBaseDataVariable &variable, const QString &browseName)
{
    const QUaNodeId parentId = variable.nodeId();
    if (parentId.namespaceIndex() != 1 || parentId.type() != QUaNodeIdType::String)
        return {};
    return QUaNodeId(1, parentId.stringId() + QLatin1Char('.') + browseName);
}

///
/// \brief Splits the text of an array or a Range into its trimmed parts.
/// \param text Parts separated by ";".
/// \return Non-empty parts.
///
QStringList parts(const QString &text)
{
    QStringList result;
    const QStringList split = text.split(QLatin1Char(';'));
    for (const QString &part : split) {
        if (!part.trimmed().isEmpty())
            result.append(part.trimmed());
    }
    return result;
}

///
/// \brief Writes a number with the C locale and the shortest digits that read back to it.
/// \param number Number to write.
/// \return Text such as "0.5".
///
QString number(double number)
{
    return QString::number(number, 'g', QLocale::FloatingPointShortest);
}

///
/// \brief Parses one EnumValueType of EnumValues.
/// \param text Value and display name, e.g. "1: On".
/// \return Structure, or invalid when the value is no integer.
///
QVariant enumValueFromText(const QString &text)
{
    static const QRegularExpression pattern(QStringLiteral("^\\s*(-?\\d+)\\s*:\\s*(.*)$"));
    const QRegularExpressionMatch match = pattern.match(text);
    if (!match.hasMatch())
        return {};
    QUaStructure value(QUaNodeId(0, 7594u));
    value.setField(QStringLiteral("Value"), match.captured(1).toLongLong());
    value.setField(QStringLiteral("DisplayName"), QVariant::fromValue(QUaLocalizedText(QString(), match.captured(2))));
    value.setField(QStringLiteral("Description"), QVariant::fromValue(QUaLocalizedText()));
    return QVariant::fromValue(value);
}

///
/// \brief Parses one scalar value of a property.
/// \param text Trimmed text.
/// \param dataType Scalar data type name.
/// \return Value, or invalid when the text is no value of the type.
///
QVariant scalarFromText(const QString &text, const QString &dataType)
{
    if (dataType == kLocalizedText)
        return QVariant::fromValue(QUaLocalizedText(QString(), text));
    if (dataType == kEUInformation) {
        QUaStructure units(QUaNodeId(0, 887u));
        units.setField(QStringLiteral("NamespaceUri"), kUnitsNamespace);
        units.setField(QStringLiteral("UnitId"), kUnknownUnitId);
        units.setField(QStringLiteral("DisplayName"), QVariant::fromValue(QUaLocalizedText(QString(), text)));
        units.setField(QStringLiteral("Description"), QVariant::fromValue(QUaLocalizedText(QString(), text)));
        return QVariant::fromValue(units);
    }
    if (dataType == kRange) {
        const QStringList limits = parts(text);
        const QVariant low = limits.size() == 2 ? ValueCast::fromText(limits.first(), QStringLiteral("Double")) : QVariant();
        const QVariant high = limits.size() == 2 ? ValueCast::fromText(limits.last(), QStringLiteral("Double")) : QVariant();
        if (!low.isValid() || !high.isValid())
            return {};
        QUaStructure range(QUaNodeId(0, 884u));
        range.setField(QStringLiteral("Low"), low.toDouble());
        range.setField(QStringLiteral("High"), high.toDouble());
        return QVariant::fromValue(range);
    }
    return ValueCast::isSupported(dataType) ? ValueCast::fromText(text, dataType) : QVariant();
}

///
/// \brief Returns the value a property of a data type starts with, so that clients read its data type from it.
/// \param dataType Data type name as PropertyInfo::dataType holds it.
/// \return Empty or zero value of the type: a 0..0 Range, units without a name, an empty text or array.
///
QVariant defaultValue(const QString &dataType)
{
    if (dataType == kRange)
        return scalarFromText(QStringLiteral("0; 0"), kRange);
    if (dataType == kEUInformation)
        return scalarFromText(QString(), kEUInformation);
    if (dataType == kLocalizedText)
        return QVariant::fromValue(QUaLocalizedText());
    if (dataType == kLocalizedText + kArraySuffix)
        return QVariant::fromValue(QList<QUaLocalizedText>());
    if (dataType.endsWith(kArraySuffix))
        return QVariantList();
    return ValueCast::defaultValue(dataType);
}

///
/// \brief Writes one scalar value of a property as text.
/// \param value Value as the server reads it.
/// \param dataType Scalar data type name.
/// \return Text, or empty for no value.
///
QString scalarToText(const QVariant &value, const QString &dataType)
{
    if (dataType == kLocalizedText)
        return value.value<QUaLocalizedText>().text();
    const QUaStructure structure = value.value<QUaStructure>();
    if (dataType == kEUInformation)
        return structure.field(QStringLiteral("DisplayName")).value<QUaLocalizedText>().text();
    if (dataType == kRange) {
        if (!structure.hasField(QStringLiteral("Low")) || !structure.hasField(QStringLiteral("High")))
            return {};
        return number(structure.field(QStringLiteral("Low")).toDouble()) + QStringLiteral("; ")
            + number(structure.field(QStringLiteral("High")).toDouble());
    }
    if (dataType == kEnumValueType) {
        if (!structure.hasField(QStringLiteral("Value")))
            return {};
        return QStringLiteral("%1: %2").arg(structure.field(QStringLiteral("Value")).toLongLong())
            .arg(structure.field(QStringLiteral("DisplayName")).value<QUaLocalizedText>().text());
    }
    switch (value.typeId()) {
    case QMetaType::Double:
    case QMetaType::Float:
        return number(value.toDouble());
    case QMetaType::Bool:
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    default:
        return value.toString();
    }
}

}

namespace VariableProperties {

///
/// \brief Lists the properties of a variable and the optional properties of data items its type declares.
/// \param server Server holding the variable.
/// \param variable Served variable.
/// \return The data item properties of OPC UA Part 8 that the type declares, e.g. EURange, EngineeringUnits and
///         InstrumentRange of an AnalogItemType, then the other properties the variable has.
///
QList<PropertyInfo> describe(const QUaServer &server, const QUaBaseDataVariable &variable)
{
    QList<PropertyInfo> result;
    QStringList listed;
    const QUaNodeId typeId = variable.typeDefinitionNodeId();
    for (const Declaration &declaration : declarations()) {
        if (!server.isSubtypeOf(typeId, declaration.typeId))
            continue;
        PropertyInfo info;
        info.browseName = declaration.browseName;
        info.dataType = declaration.dataType;
        if (const QUaProperty *property = findProperty(variable, declaration.browseName))
            info.value = toText(property->value(), info.dataType);
        info.editable = isSupported(info.dataType);
        result.append(info);
        listed.append(info.browseName);
    }
    const QList<QUaNode *> children = variable.browseChildren();
    for (const QUaNode *child : children) {
        const auto *property = qobject_cast<const QUaProperty *>(child);
        if (!property || listed.contains(child->browseName().name()))
            continue;
        PropertyInfo info;
        info.browseName = child->browseName().name();
        info.dataType = dataType(*property);
        info.value = toText(property->value(), info.dataType);
        info.editable = isSupported(info.dataType);
        result.append(info);
    }
    return result;
}

///
/// \brief Adds the data item properties that the type of a variable declares and the variable lacks, with the value
///        their data type starts with, as when every optional property of the type is instantiated.
/// \param server Server holding the variable.
/// \param variable Served variable built by the project.
///
void addDeclared(const QUaServer &server, QUaBaseDataVariable &variable)
{
    const QUaNodeId typeId = variable.typeDefinitionNodeId();
    for (const Declaration &declaration : declarations()) {
        if (!server.isSubtypeOf(typeId, declaration.typeId) || findProperty(variable, declaration.browseName))
            continue;
        QUaProperty *property = variable.addProperty(QUaQualifiedName(0, declaration.browseName),
                                                     propertyNodeId(variable, declaration.browseName));
        if (property)
            property->setValue(defaultValue(declaration.dataType));
    }
}

///
/// \brief Sets property values of a variable, adding the optional data item properties its type declares.
/// \param server Server holding the variable.
/// \param variable Served variable.
/// \param values Value texts keyed by browse name.
/// \return Why values were skipped: the variable has no such property or the text is no value of its data type.
///
QStringList apply(const QUaServer &server, QUaBaseDataVariable &variable, const QMap<QString, QString> &values)
{
    QStringList problems;
    for (auto it = values.cbegin(); it != values.cend(); ++it) {
        QUaProperty *property = findProperty(variable, it.key());
        const Declaration *declaration = declarationOf(server, variable, it.key());
        if (!property && !declaration) {
            problems.append(QCoreApplication::translate("VariableProperties", "the type declares no property '%1'")
                                .arg(it.key()));
            continue;
        }
        const QString type = declaration ? declaration->dataType : dataType(*property);
        const QVariant value = fromText(it.value(), type);
        if (!value.isValid()) {
            problems.append(QCoreApplication::translate("VariableProperties", "'%1' is not a %2 value for '%3'")
                                .arg(it.value(), type, it.key()));
            continue;
        }
        if (!property)
            property = variable.addProperty(QUaQualifiedName(0, it.key()), propertyNodeId(variable, it.key()));
        if (property)
            property->setValue(value);
    }
    return problems;
}

///
/// \brief Returns the data type name of a variable from its DataType attribute and value.
/// \param variable Served property or variable.
/// \return Name such as "Double", "Range" or "LocalizedText[]", or the node id of a data type without a name here.
///
QString dataType(const QUaBaseVariable &variable)
{
    static const QHash<QString, QString> structures {
        { QStringLiteral("i=21"), kLocalizedText }, { QStringLiteral("i=884"), kRange },
        { QStringLiteral("i=887"), kEUInformation }, { QStringLiteral("i=7594"), kEnumValueType },
    };
    QString id = variable.dataTypeNodeId();
    if (id.startsWith(QLatin1String("ns=0;")))
        id.remove(0, 5);
    const QString name = structures.value(id, ImportedNodes::dataTypeName(id));
    const bool array = variable.valueRank() > 0 || ValueCast::isArray(variable.value());
    return array ? name + kArraySuffix : name;
}

///
/// \brief Describes a property or a component variable of a variable for display; its value is not simulated.
/// \param server Server holding the node.
/// \param node Served child of a variable.
/// \return Description with the value as text for data types the simulation does not handle; a property whose
///         value can be typed has that text as initial value and is editable, other children are read-only.
///
VariableInfo describeChild(const QUaServer &server, const QUaBaseVariable &node)
{
    VariableInfo info;
    info.nodeId = node.nodeId();
    info.displayName = node.browseName().name();
    info.icon = qobject_cast<const QUaProperty *>(&node) ? QStringLiteral("property") : QString();
    info.dataType = dataType(node);
    info.value = ValueCast::isSupported(info.dataType) ? node.value() : QVariant(toText(node.value(), info.dataType));
    info.access = ModelVariables::accessText(node);
    info.quality = AddressSpace::qualityOf(node);
    info.configuredQuality = info.quality;
    info.typeName = node.typeDefinitionBrowseName().name();
    info.typeIcon = TypeCatalog::variableTypeIcon(server, node.typeDefinitionNodeId().toXmlString());
    if (const auto *variable = qobject_cast<const QUaBaseDataVariable *>(&node))
        info.properties = describe(server, *variable);
    info.property = qobject_cast<const QUaProperty *>(&node) != nullptr;
    if (info.property)
        info.initialValue = toText(node.value(), info.dataType);
    info.readOnly = !info.property || !isSupported(info.dataType);
    info.fromModel = true;
    return info;
}

///
/// \brief Parses the text of a property value.
/// \param text Text as toText() writes it: "0; 100" for a Range, the unit for EUInformation, "0: Off; 1: On" for
///        EnumValueType[] and values separated by ";" for other arrays.
/// \param dataType Data type name as PropertyInfo::dataType holds it.
/// \return Value to write, or invalid for empty text, text that is no value of the type or an unsupported type.
///
QVariant fromText(const QString &text, const QString &dataType)
{
    if (text.trimmed().isEmpty() || !isSupported(dataType))
        return {};
    if (!dataType.endsWith(kArraySuffix))
        return scalarFromText(text.trimmed(), dataType);

    const QString elementType = dataType.chopped(kArraySuffix.size());
    QVariantList elements;
    QList<QUaLocalizedText> texts;
    const QStringList elementTexts = parts(text);
    for (const QString &elementText : elementTexts) {
        const QVariant element =
            elementType == kEnumValueType ? enumValueFromText(elementText) : scalarFromText(elementText, elementType);
        if (!element.isValid())
            return {};
        elements.append(element);
        if (elementType == kLocalizedText)
            texts.append(element.value<QUaLocalizedText>());
    }
    return elementType == kLocalizedText ? QVariant::fromValue(texts) : QVariant(elements);
}

///
/// \brief Writes a property value as text; numbers use the C locale so project files load anywhere.
/// \param value Value as the server reads it.
/// \param dataType Data type name as PropertyInfo::dataType holds it.
/// \return Text that fromText() reads back, or empty for no value.
///
QString toText(const QVariant &value, const QString &dataType)
{
    if (!value.isValid())
        return {};
    if (!dataType.endsWith(kArraySuffix) || !value.canConvert<QVariantList>())
        return scalarToText(value, dataType);
    const QString elementType = dataType.chopped(kArraySuffix.size());
    QStringList elements;
    const QVariantList list = value.toList();
    for (const QVariant &element : list)
        elements.append(scalarToText(element, elementType));
    return elements.join(QStringLiteral("; "));
}

///
/// \brief Tells whether property values of a data type can be typed as text.
/// \param dataType Data type name as PropertyInfo::dataType holds it.
/// \return True for the built-in types the simulation supports, LocalizedText, Range, EUInformation and arrays of
///         them, and EnumValueType[].
///
bool isSupported(const QString &dataType)
{
    const bool array = dataType.endsWith(kArraySuffix);
    const QString elementType = array ? dataType.chopped(kArraySuffix.size()) : dataType;
    if (elementType == kEnumValueType)
        return array;
    return elementType == kLocalizedText || elementType == kRange || elementType == kEUInformation
        || ValueCast::isSupported(elementType);
}

}
