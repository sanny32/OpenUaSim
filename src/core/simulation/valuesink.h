// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file valuesink.h
/// \brief Declares the receiver of simulated values.
///

#pragma once

#include <QString>
#include <QVariant>

///
/// \brief Receives the values the simulation engine generates.
///
class ValueSink
{
public:
    virtual ~ValueSink() = default;

    ///
    /// \brief Stores a generated value.
    /// \param nodeId Variable node id.
    /// \param value Value already converted to the variable's data type.
    /// \return False when the variable does not exist.
    ///
    virtual bool setValue(const QString &nodeId, const QVariant &value) = 0;
};
