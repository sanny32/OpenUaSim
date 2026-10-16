// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file typecatalog.h
/// \brief Declares the standard types the simulator's nodes are instances of.
///

#pragma once

#include <QHash>
#include <QList>

#include "data/treenode.h"
#include "data/typeinfo.h"

namespace TypeCatalog {

///
/// \brief Returns the standard object and variable types of namespace 0 the project nodes use.
/// \return Types keyed by node id, with HasSubtype counts.
///
QHash<QString, TypeInfo> types();

///
/// \brief Builds the navigation tree of types().
/// \return One "Types" root holding the BaseObjectType and BaseVariableType hierarchies,
///         with FolderType current.
///
QList<TreeNode> tree();

}
