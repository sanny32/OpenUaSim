// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file methodinfo.h
/// \brief Declares the description of an OPC UA method and its arguments.
///

#pragma once

#include <QList>
#include <QString>

#include "projectconfig.h"

///
/// \brief Describes a method of an object in the address space.
///
struct MethodInfo
{
    ///
    /// \brief Who defines what the method does.
    ///
    enum class Kind {
        Standard, ///< The server implements it; nothing can be set.
        Project,  ///< A method the project adds, with an action on the variables next to it.
        Model     ///< A method of an imported model, answering with a configurable response.
    };

    /// \brief Server node id.
    QString nodeId;
    /// \brief Display name.
    QString displayName;
    /// \brief Description of the method.
    QString description;
    QStringList namespaces;
    /// \brief Who defines what the method does.
    Kind kind = Kind::Standard;
    /// \brief Action of a project method.
    NodeDefinition::MethodAction action = NodeDefinition::MethodAction::Reset;
    /// \brief Answer of a model method to the calls of clients.
    MethodResponse response;
    /// \brief Input arguments.
    QList<ArgumentInfo> inputs;
    /// \brief Output arguments, in the order of MethodResponse::outputs.
    QList<ArgumentInfo> outputs;
};
