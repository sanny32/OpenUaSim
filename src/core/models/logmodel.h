// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file logmodel.h
/// \brief Declares the server log table model.
///

#pragma once

#include "data/logentry.h"
#include "data/statuskind.h"
#include "itemtablemodel.h"

///
/// \brief Table of server log entries: time, level, category and message.
///
class LogModel : public ItemTableModel<LogEntry>
{
    Q_OBJECT

public:
    ///
    /// \brief Columns exposed by the model.
    ///
    enum Column { ColTime, ColLevel, ColCategory, ColMessage, ColCount };

    ///
    /// \brief Constructs an empty model.
    /// \param parent Owning QObject.
    ///
    explicit LogModel(QObject *parent = nullptr);

    ///
    /// \brief Returns the fixed column count.
    /// \param parent Parent index; non-root parents have no columns.
    /// \return Column count, or 0 for non-root parents.
    ///
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns cell text and the level indicator.
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
    /// \brief Returns the display text of a level.
    /// \param level Level to describe.
    /// \return "INFO", "WARNING" or "ERROR".
    ///
    static QString levelText(LogEntry::Level level);

    ///
    /// \brief Returns the indicator state of a level.
    /// \param level Level to describe.
    /// \return Info, Warning or Error.
    ///
    static StatusKind levelStatus(LogEntry::Level level);

    ///
    /// \brief Returns the display text of a category.
    /// \param category Category to describe.
    /// \return "Server", "Session", "Security" or "Data Changes".
    ///
    static QString categoryText(LogEntry::Category category);
};
