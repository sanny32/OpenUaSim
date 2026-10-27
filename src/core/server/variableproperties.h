// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file variableproperties.h
/// \brief Declares the properties of variables, such as the EURange of an analog item, and their text form.
///

#pragma once

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "data/variableinfo.h"

class QUaBaseDataVariable;
class QUaBaseVariable;
class QUaNode;
class QUaServer;

namespace VariableProperties {

///
/// \brief Lists the properties of a variable and the optional properties of data items its type declares.
/// \param server Server holding the variable.
/// \param variable Served variable.
/// \return The data item properties of OPC UA Part 8 that the type declares, e.g. EURange, EngineeringUnits and
///         InstrumentRange of an AnalogItemType, then the other properties the variable has.
///
QList<PropertyInfo> describe(const QUaServer &server, const QUaBaseDataVariable &variable);

///
/// \brief Adds the data item properties that the type of a variable declares and the variable lacks, with the value
///        their data type starts with, as when every optional property of the type is instantiated.
/// \param server Server holding the variable.
/// \param variable Served variable built by the project.
///
void addDeclared(const QUaServer &server, QUaBaseDataVariable &variable);

///
/// \brief Sets property values of a variable, adding the optional data item properties its type declares.
/// \param server Server holding the variable.
/// \param variable Served variable.
/// \param values Value texts keyed by browse name.
/// \return Why values were skipped: the variable has no such property or the text is no value of its data type.
///
QStringList apply(const QUaServer &server, QUaBaseDataVariable &variable, const QMap<QString, QString> &values);

///
/// \brief Publishes the image of a variable as its Icon property of OPC UA Part 3, which clients may show next to it.
/// \param variable Served variable.
/// \param png PNG image; an empty one leaves the variable as it is, keeping an Icon a model gives it.
///
void setIcon(QUaBaseDataVariable &variable, const QByteArray &png);

///
/// \brief Removes the Icon property of a variable, so clients show their own icon for it again.
/// \param variable Served variable.
///
void removeIcon(QUaBaseDataVariable &variable);

///
/// \brief Tells whether a node is the Icon property of its variable, which the icon setting of the variable stands for.
/// \param node Child of a variable.
/// \return True for a property named Icon in namespace 0.
///
bool isIcon(const QUaNode &node);

///
/// \brief Returns the data type name of a variable from its DataType attribute and value.
/// \param variable Served property or variable.
/// \return Name such as "Double", "Range" or "LocalizedText[]", or the node id of a data type without a name here.
///
QString dataType(const QUaBaseVariable &variable);

///
/// \brief Describes a property or a component variable of a variable for display; its value is not simulated.
/// \param server Server holding the node.
/// \param node Served child of a variable.
/// \return Description with the value as text for data types the simulation does not handle; a property whose
///         value can be typed has that text as initial value and is editable, other children are read-only.
///
VariableInfo describeChild(const QUaServer &server, const QUaBaseVariable &node);

///
/// \brief Parses the text of a property value.
/// \param text Text as toText() writes it: "0; 100" for a Range, the unit for EUInformation, "0: Off; 1: On" for
///        EnumValueType[] and values separated by ";" for other arrays.
/// \param dataType Data type name as PropertyInfo::dataType holds it.
/// \return Value to write, or invalid for empty text, text that is no value of the type or an unsupported type.
///
QVariant fromText(const QString &text, const QString &dataType);

///
/// \brief Writes a property value as text; numbers use the C locale so project files load anywhere.
/// \param value Value as the server reads it.
/// \param dataType Data type name as PropertyInfo::dataType holds it.
/// \return Text that fromText() reads back, or empty for no value.
///
QString toText(const QVariant &value, const QString &dataType);

///
/// \brief Tells whether property values of a data type can be typed as text.
/// \param dataType Data type name as PropertyInfo::dataType holds it.
/// \return True for the built-in types the simulation supports, LocalizedText, Range, EUInformation and arrays of
///         them, and EnumValueType[].
///
bool isSupported(const QString &dataType);

}
