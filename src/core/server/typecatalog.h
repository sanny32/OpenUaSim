// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file typecatalog.h
/// \brief Declares the type hierarchy of the server: standard types and the types of imported models.
///

#pragma once

#include <QHash>
#include <QList>

#include "data/projectconfig.h"
#include "data/treenode.h"
#include "data/typeinfo.h"

class QUaServer;

namespace TypeCatalog {

///
/// \brief Selects an icon by the nearest recognized type in the supertype chain.
/// \param types Types keyed by server node id.
/// \param typeId Type whose family determines the icon.
/// \return Icon resource name, with a generic fallback for unknown types.
///
QString iconName(const QHash<QString, TypeInfo> &types, const QString &typeId);

///
/// \brief Reads the object, variable, data and reference types of a server.
/// \param server Server holding the standard types and the types of imported models.
/// \return Types keyed by node id, e.g. "i=61" or "ns=2;i=1000", with their supertype, category, variable value
///         constraints, declared children and forward references counted by reference type.
///
QHash<QString, TypeInfo> types(const QUaServer &server);

///
/// \brief Reads the value constraints of a variable type without creating an instance.
/// \param server Server holding the type and its data type hierarchy.
/// \param typeId Server node id of the variable type.
/// \return Type description; an empty dataTypeId means the type has no readable variable attributes.
///
TypeInfo variableType(const QUaServer &server, const QString &typeId);

///
/// \brief Checks whether a simulated value satisfies a variable type's constraints.
/// \param type Variable type with its value constraints.
/// \param dataType Built-in data type of the value or array elements.
/// \param arrayLength Zero for a scalar, otherwise the one-dimensional array length.
/// \return Problem text, or empty for a supported, compatible value shape.
///
QString variableProblem(const TypeInfo &type, const QString &dataType, int arrayLength);

///
/// \brief Checks variable type constraints throughout a node definition before creating or changing nodes.
/// \param server Server holding the standard and imported types.
/// \param node Definition whose variables are checked; portable type ids are resolved against the server.
/// \return First problem text, or empty when every variable has a compatible value shape.
///
QString variableProblem(const QUaServer &server, const NodeDefinition &node);

///
/// \brief Tells whether a type is shown without the full standard type system.
/// \param type Type to check.
/// \return True for a type outside namespace 0, a root of a hierarchy, a type of the project nodes or a data type
///         the simulation writes.
///
bool isRelevant(const TypeInfo &type);

///
/// \brief Builds the navigation tree of types.
/// \param types Types keyed by node id, as types() returns them.
/// \param allStandardTypes Whether every type is shown; otherwise only relevant types and their supertypes.
/// \return One "Types" root holding a hierarchy per root type, siblings sorted by name, with FolderType current.
///
QList<TreeNode> tree(const QHash<QString, TypeInfo> &types, bool allStandardTypes);

///
/// \brief Lists the types a new node of a kind can be an instance of.
/// \param types Types keyed by node id, as types() returns them.
/// \param kind Folder, object or variable; methods have no type.
/// \return The default type first (FolderType, BaseObjectType or BaseDataVariableType), then its concrete subtypes
///         sorted by name: those of imported models and, for variables, the standard data item types. Event types
///         and, for objects, folder types are left out.
///
QList<TypeInfo> instanceTypes(const QHash<QString, TypeInfo> &types, NodeDefinition::Kind kind);

}
