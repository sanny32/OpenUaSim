// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file variablesmodel.cpp
/// \brief Implements the table model of an object's variables.
///

#include "formatting.h"
#include "itemroles.h"
#include "variablesmodel.h"

///
/// \brief Constructs an empty model.
/// \param parent Owning QObject.
///
VariablesModel::VariablesModel(QObject *parent)
    : ItemTableModel(parent)
{
}

///
/// \brief Returns the fixed column count.
/// \param parent Parent index; non-root parents have no columns.
/// \return Column count, or 0 for non-root parents.
///
int VariablesModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColCount;
}

///
/// \brief Returns cell text, the variable icon, and the quality indicator.
/// \param index Cell to query.
/// \param role Requested data role.
/// \return Value for the role, or an invalid variant.
///
QVariant VariablesModel::data(const QModelIndex &index, int role) const
{
    if (!hasItem(index))
        return {};
    const VariableInfo &variable = item(index.row());

    if (role == ItemRoles::Id)
        return variable.nodeId;

    switch (index.column()) {
    case ColName:
        if (role == Qt::DisplayRole) return variable.displayName;
        if (role == ItemRoles::IconName) return variable.icon;
        break;
    case ColValue:
        if (role == Qt::DisplayRole) return Formatting::value(variable.value);
        break;
    case ColDataType:
        if (role == Qt::DisplayRole) return variable.dataType;
        break;
    case ColQuality:
        if (role == Qt::DisplayRole) return qualityText(variable.quality);
        if (role == ItemRoles::Status) return ItemRoles::statusValue(qualityStatus(variable.quality));
        break;
    default:
        break;
    }
    return {};
}

///
/// \brief Returns the column titles.
/// \param section Column index.
/// \param orientation Header orientation.
/// \param role Requested data role.
/// \return Column title, or the base implementation otherwise.
///
QVariant VariablesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return ItemTableModel::headerData(section, orientation, role);
    switch (section) {
    case ColName:     return tr("Name");
    case ColValue:    return tr("Value");
    case ColDataType: return tr("Data Type");
    case ColQuality:  return tr("Quality");
    default:          return {};
    }
}

///
/// \brief Finds the row of a variable.
/// \param nodeId Variable node id.
/// \return Row number, or -1 when the variable is not listed.
///
int VariablesModel::rowOf(const QString &nodeId) const
{
    for (int row = 0; row < items().size(); ++row) {
        if (items().at(row).nodeId == nodeId)
            return row;
    }
    return -1;
}

///
/// \brief Updates the values of the listed variables.
/// \param values New values keyed by node id; variables not listed are ignored.
///
void VariablesModel::updateValues(const QHash<QString, QVariant> &values)
{
    for (int row = 0; row < items().size(); ++row) {
        const auto value = values.constFind(item(row).nodeId);
        if (value == values.constEnd())
            continue;
        VariableInfo variable = item(row);
        variable.value = value.value();
        setItem(row, variable);
    }
}

///
/// \brief Returns the display text of a quality.
/// \param quality Quality to describe.
/// \return "Good", "Uncertain" or "Bad".
///
QString VariablesModel::qualityText(VariableInfo::Quality quality)
{
    switch (quality) {
    case VariableInfo::Quality::Good:      return tr("Good");
    case VariableInfo::Quality::Uncertain: return tr("Uncertain");
    case VariableInfo::Quality::Bad:       return tr("Bad");
    }
    return {};
}

///
/// \brief Returns the indicator state of a quality.
/// \param quality Quality to describe.
/// \return Good, Warning or Error.
///
StatusKind VariablesModel::qualityStatus(VariableInfo::Quality quality)
{
    switch (quality) {
    case VariableInfo::Quality::Good:      return StatusKind::Good;
    case VariableInfo::Quality::Uncertain: return StatusKind::Warning;
    case VariableInfo::Quality::Bad:       return StatusKind::Error;
    }
    return StatusKind::None;
}
