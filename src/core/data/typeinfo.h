// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file typeinfo.h
/// \brief Declares the description of an OPC UA type node.
///

#pragma once

#include <QList>
#include <QString>
#include <QStringList>

///
/// \brief Number of references of one reference type held by a node.
///
struct ReferenceCount
{
    /// \brief Reference type browse name.
    QString referenceType;
    /// \brief Number of references of that type.
    int count = 0;
};

///
/// \brief Describes one type of the server type system.
///
struct TypeInfo
{
    /// \brief Browse name.
    QString browseName;
    /// \brief String form of the node identifier.
    QString nodeId;
    /// \brief Node identifier of the supertype; empty for a root type.
    QString baseTypeId;
    /// \brief Node class name, e.g. "ObjectType".
    QString category;
    /// \brief Number of child nodes declared by the type.
    int children = 0;
    /// \brief Whether the type is abstract.
    bool isAbstract = false;
    /// \brief Required data type node id for a variable type; empty when no constraints were read.
    QString dataTypeId;
    /// \brief Built-in data types the simulator can write as this variable type.
    QStringList valueDataTypes;
    /// \brief OPC UA ValueRank constraint; -2 permits scalars and arrays.
    qint32 valueRank = -2;
    /// \brief Required array lengths; zero permits any length in that dimension.
    QList<quint32> arrayDimensions;
    /// \brief Free-form description.
    QString description;
    /// \brief Reference counts grouped by reference type.
    QList<ReferenceCount> references;
};
