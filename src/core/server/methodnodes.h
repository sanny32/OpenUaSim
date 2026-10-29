// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

/// \file methodnodes.h
/// \brief Declares creation of methods with editable signatures.

#pragma once
#include "data/projectconfig.h"

class QUaServer;

namespace MethodNodes {

///
/// \brief Adds a project method with its configured signature using the NodeSet importer.
/// \param server Server receiving the method.
/// \param parentId Existing parent object node id.
/// \param method Definition of a scenario method.
/// \return Import error, or empty on success.
///
QString add(QUaServer &server, const QString &parentId, const NodeDefinition &method);

}
