// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectnodes.h
/// \brief Declares helpers over the node tree of a project.
///

#pragma once

#include <QList>

#include "data/projectconfig.h"

namespace ProjectNodes {

///
/// \brief Gives every node without a node id one derived from its browse path.
/// \param nodes Node tree to complete in place.
/// \return Number of node ids assigned; ids such as "ns=1;s=Line.Motor.Speed" get a numeric
///         suffix when the path is already taken.
///
int assignMissingNodeIds(QList<NodeDefinition> &nodes);

///
/// \brief Counts the nodes of a tree.
/// \param nodes Node tree.
/// \return Number of nodes at every depth.
///
int count(const QList<NodeDefinition> &nodes);

}
