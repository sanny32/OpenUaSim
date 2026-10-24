// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file variablesmodel.cpp
/// \brief Implements the hierarchical model of an object's variables.
///

#include "formatting.h"
#include "itemroles.h"
#include "variablesmodel.h"

///
/// \brief Constructs an empty model.
/// \param parent Owning QObject.
///
VariablesModel::VariablesModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

///
/// \brief Returns the fixed column count.
/// \param parent Parent index.
/// \return Column count.
///
int VariablesModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return ColCount;
}

///
/// \brief Returns cell text, the variable icon, and the quality and alarm indicators.
/// \param index Cell to query.
/// \param role Requested data role.
/// \return Value for the role, or an invalid variant.
///
QVariant VariablesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.model() != this || index.internalId() == 0
        || index.internalId() > quintptr(_items.size()))
        return {};
    const VariableInfo &variable = item(int(index.internalId()) - 1);

    if (role == ItemRoles::Id)
        return variable.nodeId;

    switch (index.column()) {
    case ColName:
        if (role == Qt::DisplayRole) {
            const QString location = _locations.value(variable.nodeId);
            return location.isEmpty() ? variable.displayName : location + QLatin1Char('/') + variable.displayName;
        }
        if (role == ItemRoles::IconName) return variable.icon;
        break;
    case ColValue:
        if (role == Qt::DisplayRole) return Formatting::value(variable.value);
        break;
    case ColDataType:
        if (role == Qt::DisplayRole)
            return variable.arrayLength > 0
                ? QStringLiteral("%1[%2]").arg(variable.shownDataType()).arg(variable.arrayLength)
                : variable.shownDataType();
        break;
    case ColNodeType:
        if (role == Qt::DisplayRole || role == Qt::ToolTipRole) return variable.typeName;
        break;
    case ColQuality:
        if (role == Qt::DisplayRole) return qualityText(variable.quality);
        if (role == ItemRoles::Status) return ItemRoles::statusValue(qualityStatus(variable.quality));
        break;
    case ColAlarm:
        if (!variable.alarm.enabled)
            break;
        if (role == Qt::DisplayRole) return variable.alarmState.isEmpty() ? tr("Normal") : variable.alarmState;
        if (role == ItemRoles::Status)
            return ItemRoles::statusValue(variable.alarmState.isEmpty() ? StatusKind::Good : StatusKind::Error);
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
        return QAbstractItemModel::headerData(section, orientation, role);
    switch (section) {
    case ColName:     return tr("Name");
    case ColValue:    return tr("Value");
    case ColDataType: return tr("Data Type");
    case ColNodeType: return tr("Node Type");
    case ColQuality:  return tr("Quality");
    case ColAlarm:    return tr("Alarm");
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
/// \brief Replaces the listed variables.
/// \param variables Variables in row order.
/// \param locations Owner path shown before the name, keyed by variable node id; omitted for none.
/// \param owners Parent variable ids keyed by child node id; absent parents leave rows at the root.
///
void VariablesModel::setVariables(const QList<VariableInfo> &variables, const QHash<QString, QString> &locations,
                                  const QHash<QString, QString> &owners)
{
    beginResetModel();
    _locations = locations;
    _items = variables;
    _parents.clear();
    _children.clear();
    for (const VariableInfo &variable : variables) {
        const QString owner = owners.value(variable.nodeId);
        _parents.append(owner.isEmpty() ? -1 : rowOf(owner));
    }
    for (int row = 0; row < _items.size(); ++row)
        _children[_parents.at(row)].append(row);
    endResetModel();
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
/// \return Severity, followed by the reason of a specific status code, e.g. "Bad (sensor failure)".
///
QString VariablesModel::qualityText(VariableInfo::Quality quality)
{
    using Q = VariableInfo::Quality;
    switch (quality) {
    case Q::Good:                                    return tr("Good");
    case Q::GoodLocalOverride:                       return tr("Good (local override)");
    case Q::Uncertain:                               return tr("Uncertain");
    case Q::UncertainNoCommunicationLastUsableValue: return tr("Uncertain (no communication, last usable value)");
    case Q::UncertainLastUsableValue:                return tr("Uncertain (last usable value)");
    case Q::UncertainSubstituteValue:                return tr("Uncertain (substitute value)");
    case Q::UncertainInitialValue:                   return tr("Uncertain (initial value)");
    case Q::UncertainSensorNotAccurate:              return tr("Uncertain (sensor not accurate)");
    case Q::UncertainEngineeringUnitsExceeded:       return tr("Uncertain (engineering units exceeded)");
    case Q::UncertainSubNormal:                      return tr("Uncertain (sub-normal)");
    case Q::Bad:                                     return tr("Bad");
    case Q::BadConfigurationError:                   return tr("Bad (configuration error)");
    case Q::BadNotConnected:                         return tr("Bad (not connected)");
    case Q::BadDeviceFailure:                        return tr("Bad (device failure)");
    case Q::BadSensorFailure:                        return tr("Bad (sensor failure)");
    case Q::BadOutOfService:                         return tr("Bad (out of service)");
    case Q::BadNoCommunication:                      return tr("Bad (no communication)");
    case Q::BadWaitingForInitialData:                return tr("Bad (waiting for initial data)");
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
    switch (VariableInfo::severityOf(quint32(quality))) {
    case VariableInfo::Quality::Good:      return StatusKind::Good;
    case VariableInfo::Quality::Uncertain: return StatusKind::Warning;
    default:                               return StatusKind::Error;
    }
    return StatusKind::None;
}

///
/// \brief Returns the number of children of a variable or the root.
///
int VariablesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return 0;
    return int(_children.value(parent.isValid() ? int(parent.internalId()) - 1 : -1).size());
}

///
/// \brief Returns a cell among the children of a variable or the root.
///
QModelIndex VariablesModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return {};
    const int source = _children.value(parent.isValid() ? int(parent.internalId()) - 1 : -1).at(row);
    return createIndex(row, column, quintptr(source + 1));
}

///
/// \brief Returns the variable owning a child row.
///
QModelIndex VariablesModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};
    return indexForRow(_parents.at(int(index.internalId()) - 1));
}

///
/// \brief Maps a stored item to its hierarchical cell.
///
QModelIndex VariablesModel::indexForRow(int row, int column) const
{
    if (row < 0 || row >= _items.size())
        return {};
    return createIndex(int(_children.value(_parents.at(row)).indexOf(row)), column, quintptr(row + 1));
}

///
/// \brief Replaces the items with a flat list.
///
void VariablesModel::setItems(const QList<VariableInfo> &items)
{
    setVariables(items);
}

///
/// \brief Returns the stored items in source order.
///
const QList<VariableInfo> &VariablesModel::items() const
{
    return _items;
}

///
/// \brief Returns a stored item by its source row.
///
const VariableInfo &VariablesModel::item(int row) const
{
    return _items.at(row);
}

///
/// \brief Updates a stored item without changing its parent.
///
void VariablesModel::setItem(int row, const VariableInfo &item)
{
    _items[row] = item;
    emit dataChanged(indexForRow(row), indexForRow(row, ColCount - 1));
}

///
/// \brief Removes all stored items.
///
void VariablesModel::clear()
{
    setItems({});
}

///
/// \brief Refreshes translated headers and cells.
///
void VariablesModel::retranslate()
{
    emit headerDataChanged(Qt::Horizontal, 0, ColCount - 1);
    for (int row = 0; row < _items.size(); ++row)
        emit dataChanged(indexForRow(row), indexForRow(row, ColCount - 1));
}
