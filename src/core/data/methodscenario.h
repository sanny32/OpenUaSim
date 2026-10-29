// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

/// \file methodscenario.h
/// \brief Declares method signatures, value sources and ordered variable assignments.

#pragma once

#include "variableinfo.h"
#include <QList>
#include <QMetaType>

/// \brief Input or output argument of a method.
struct ArgumentInfo
{
    /// \brief Argument name.
    QString name;
    /// \brief Display type, including [] for an array.
    QString dataType;
    /// \brief Built-in type used to parse text; empty for unsupported data types.
    QString valueType;
    bool array = false;
    /// \brief Argument description.
    QString description;
    /// \brief Declared fixed dimensions; zero denotes a variable-length dimension.
    QList<quint32> arrayDimensions;

    bool operator==(const ArgumentInfo &other) const
    {
        return name == other.name && dataType == other.dataType && valueType == other.valueType
            && array == other.array && description == other.description && arrayDimensions == other.arrayDimensions;
    }
};

struct MethodValueSource
{
    enum class Kind { Constant, Input, Variable, Simulation };
    Kind kind = Kind::Constant;
    QString value;
    int inputIndex = 0;
    /// \brief Namespace URI reference for imported nodes, or ns=1 for project nodes.
    QString variableId;
    SimulationSettings simulation;

    bool operator==(const MethodValueSource &other) const
    {
        return kind == other.kind && value == other.value && inputIndex == other.inputIndex
            && variableId == other.variableId && simulation.function == other.simulation.function
            && simulation.intervalMs == other.simulation.intervalMs && simulation.periodMs == other.simulation.periodMs
            && simulation.amplitude == other.simulation.amplitude && simulation.offset == other.simulation.offset
            && simulation.values == other.simulation.values;
    }
};

struct MethodSetAction
{
    bool enabled = true;
    /// \brief Namespace URI reference for imported targets, or ns=1 for project targets.
    QString targetId;
    MethodValueSource source;

    bool operator==(const MethodSetAction &other) const
    {
        return enabled == other.enabled && targetId == other.targetId && source == other.source;
    }
};

struct MethodResponse
{
    quint32 statusCode = 0;
    /// \brief Legacy constant texts; sources take precedence at the same output index.
    QStringList outputs;
    QList<MethodValueSource> sources;
    QList<MethodSetAction> actions;

    bool operator==(const MethodResponse &other) const
    {
        return statusCode == other.statusCode && outputs == other.outputs && sources == other.sources
            && actions == other.actions;
    }
    bool operator!=(const MethodResponse &other) const { return !(*this == other); }
};

Q_DECLARE_METATYPE(MethodValueSource)
