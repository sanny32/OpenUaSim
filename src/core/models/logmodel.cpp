// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file logmodel.cpp
/// \brief Implements the server log table model.
///

#include "formatting.h"
#include "itemroles.h"
#include "logmodel.h"

///
/// \brief Constructs an empty model.
/// \param parent Owning QObject.
///
LogModel::LogModel(QObject *parent)
    : ItemTableModel(parent)
{
}

///
/// \brief Returns the fixed column count.
/// \param parent Parent index; non-root parents have no columns.
/// \return Column count, or 0 for non-root parents.
///
int LogModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColCount;
}

///
/// \brief Returns cell text and the level indicator.
/// \param index Cell to query.
/// \param role Requested data role.
/// \return Value for the role, or an invalid variant.
///
QVariant LogModel::data(const QModelIndex &index, int role) const
{
    if (!hasItem(index))
        return {};
    const LogEntry &entry = item(index.row());

    if (role == ItemRoles::Status && index.column() == ColLevel)
        return ItemRoles::statusValue(levelStatus(entry.level));
    if (role != Qt::DisplayRole)
        return {};

    switch (index.column()) {
    case ColTime:     return Formatting::preciseDateTime(entry.time);
    case ColLevel:    return levelText(entry.level);
    case ColCategory: return categoryText(entry.category);
    case ColMessage:  return entry.message;
    default:          return {};
    }
}

///
/// \brief Returns the column titles.
/// \param section Column index.
/// \param orientation Header orientation.
/// \param role Requested data role.
/// \return Column title, or the base implementation otherwise.
///
QVariant LogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return ItemTableModel::headerData(section, orientation, role);
    switch (section) {
    case ColTime:     return tr("Time");
    case ColLevel:    return tr("Level");
    case ColCategory: return tr("Category");
    case ColMessage:  return tr("Message");
    default:          return {};
    }
}

///
/// \brief Returns the display text of a level.
/// \param level Level to describe.
/// \return "INFO", "WARNING" or "ERROR".
///
QString LogModel::levelText(LogEntry::Level level)
{
    switch (level) {
    case LogEntry::Level::Info:    return QStringLiteral("INFO");
    case LogEntry::Level::Warning: return QStringLiteral("WARNING");
    case LogEntry::Level::Error:   return QStringLiteral("ERROR");
    }
    return {};
}

///
/// \brief Returns the indicator state of a level.
/// \param level Level to describe.
/// \return Info, Warning or Error.
///
StatusKind LogModel::levelStatus(LogEntry::Level level)
{
    switch (level) {
    case LogEntry::Level::Info:    return StatusKind::Info;
    case LogEntry::Level::Warning: return StatusKind::Warning;
    case LogEntry::Level::Error:   return StatusKind::Error;
    }
    return StatusKind::None;
}

///
/// \brief Returns the display text of a category.
/// \param category Category to describe.
/// \return "Server", "Session", "Security" or "Data Changes".
///
QString LogModel::categoryText(LogEntry::Category category)
{
    switch (category) {
    case LogEntry::Category::Server:      return tr("Server");
    case LogEntry::Category::Session:     return tr("Session");
    case LogEntry::Category::Security:    return tr("Security");
    case LogEntry::Category::DataChanges: return tr("Data Changes");
    }
    return {};
}
