// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file variablesmodel.h
/// \brief Declares the hierarchical model of an object's variables.
///

#pragma once

#include <QHash>

#include "data/statuskind.h"
#include "data/variableinfo.h"
#include "itemtreemodel.h"

///
/// \brief Hierarchy of the variables of one object: name, value, data type, node type, quality and alarm state.
///
class VariablesModel : public ItemTreeModel<VariableInfo>
{
    Q_OBJECT

public:
    ///
    /// \brief Columns exposed by the model.
    ///
    enum Column { ColName, ColValue, ColDataType, ColNodeType, ColQuality, ColAlarm, ColCount };

    ///
    /// \brief Constructs an empty model.
    /// \param parent Owning QObject.
    ///
    explicit VariablesModel(QObject *parent = nullptr);

    ///
    /// \brief Returns the fixed column count.
    /// \param parent Parent index.
    /// \return Column count.
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
    /// \brief Replaces the listed variables.
    /// \param variables Variables in row order.
    /// \param locations Owner path shown before the name, keyed by variable node id; omitted for none.
    /// \param owners Parent variable ids keyed by child node id; absent parents leave rows at the root.
    ///
    void setVariables(const QList<VariableInfo> &variables, const QHash<QString, QString> &locations = {},
                      const QHash<QString, QString> &owners = {});

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

private:
    QHash<QString, QString> _locations;
};
