// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file variablesmodel.h
/// \brief Declares the table model of an object's variables.
///

#pragma once

#include <QHash>

#include "data/statuskind.h"
#include "data/variableinfo.h"
#include "itemtablemodel.h"

///
/// \brief Table of the variables of one object: name, value, data type, quality and alarm state.
///
class VariablesModel : public ItemTableModel<VariableInfo>
{
    Q_OBJECT

public:
    ///
    /// \brief Columns exposed by the model.
    ///
    enum Column { ColName, ColValue, ColDataType, ColQuality, ColAlarm, ColCount };

    ///
    /// \brief Constructs an empty model.
    /// \param parent Owning QObject.
    ///
    explicit VariablesModel(QObject *parent = nullptr);

    ///
    /// \brief Returns the fixed column count.
    /// \param parent Parent index; non-root parents have no columns.
    /// \return Column count, or 0 for non-root parents.
    ///
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns cell text, the variable icon, and the quality and alarm indicators.
    /// \param index Cell to query.
    /// \param role Requested data role.
    /// \return Value for the role, or an invalid variant.
    ///
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    ///
    /// \brief Returns the column titles.
    /// \param section Column index.
    /// \param orientation Header orientation.
    /// \param role Requested data role.
    /// \return Column title, or the base implementation otherwise.
    ///
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    ///
    /// \brief Finds the row of a variable.
    /// \param nodeId Variable node id.
    /// \return Row number, or -1 when the variable is not listed.
    ///
    int rowOf(const QString &nodeId) const;

    ///
    /// \brief Updates the values of the listed variables.
    /// \param values New values keyed by node id; variables not listed are ignored.
    ///
    void updateValues(const QHash<QString, QVariant> &values);

    ///
    /// \brief Returns the display text of a quality.
    /// \param quality Quality to describe.
    /// \return Severity, followed by the reason of a specific status code, e.g. "Bad (sensor failure)".
    ///
    static QString qualityText(VariableInfo::Quality quality);

    ///
    /// \brief Returns the indicator state of a quality.
    /// \param quality Quality to describe.
    /// \return Good, Warning or Error.
    ///
    static StatusKind qualityStatus(VariableInfo::Quality quality);
};
