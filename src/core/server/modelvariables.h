// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file modelvariables.h
/// \brief Declares the mapping between the project settings of model variables and the server nodes.
///

#pragma once

#include <QList>
#include <QStringList>

#include "data/projectconfig.h"

class QUaBaseDataVariable;
class QUaBaseVariable;
class QUaServer;

namespace ModelVariables {

///
/// \brief Returns the access level text of a variable.
/// \param variable Variable to describe.
/// \return "Read", "Write" or "Read/Write".
///
QString accessText(const QUaBaseVariable &variable);

///
/// \brief Returns the variable of an imported model that a server node id names.
/// \param server Server holding the models.
/// \param nodeId Node id with a server namespace index, e.g. "ns=2;i=1003".
/// \return Variable, or null for an unknown node or a node of namespace 0 or 1.
///
QUaBaseDataVariable *find(QUaServer &server, const QString &nodeId);

///
/// \brief Describes a model variable as it is defined by its model.
/// \param server Server holding the models.
/// \param variable Variable of an imported model.
/// \return Settings with the namespace URI, the identifier within it, the browse name, data type, array
///         length, access and historizing of the variable, Good quality and an invalid initial value; the data
///         type is the built-in one its values are encoded with.
///
ModelVariable describe(const QUaServer &server, const QUaBaseDataVariable &variable);

///
/// \brief Returns the built-in data type the values of a model variable are encoded with.
/// \param server Server holding the model.
/// \param variable Variable of an imported model.
/// \return Name such as "Double", "Int32" for an enumeration, or the data type node id for other types.
///
QString valueDataType(const QUaServer &server, const QUaBaseDataVariable &variable);

///
/// \brief Returns the name of a data type a model defines.
/// \param server Server holding the model.
/// \param variable Variable of an imported model.
/// \return Browse name of the data type, e.g. "ConveyorStateEnumeration"; empty for a built-in type.
///
QString dataTypeName(const QUaServer &server, const QUaBaseDataVariable &variable);

///
/// \brief Tells whether the simulation can drive a model variable.
/// \param server Server holding the model.
/// \param variable Variable of an imported model.
/// \return True for a scalar or one-dimensional array of a supported built-in data type or of an enumeration.
///
bool isEditable(const QUaServer &server, const QUaBaseDataVariable &variable);

///
/// \brief Finds the stored settings of a model variable.
/// \param variables Settings kept by the project.
/// \param namespaceUri URI of the model namespace.
/// \param identifier Node id within the namespace, e.g. "i=1003".
/// \return Index into variables, or -1 when the project keeps none.
///
qsizetype indexOf(const QList<ModelVariable> &variables, const QString &namespaceUri, const QString &identifier);

///
/// \brief Turns the stored settings into variable definitions of the served model nodes.
/// \param variables Settings kept by the project.
/// \param server Server holding the models.
/// \param problems Receives why settings were skipped: an unloaded namespace, an unknown node or a variable
///        the simulation cannot drive; may be null.
/// \return Definitions with server node ids, and the browse names, data types and array lengths of the model.
///
QList<NodeDefinition> resolve(const QList<ModelVariable> &variables, QUaServer &server,
                              QStringList *problems = nullptr);

}
