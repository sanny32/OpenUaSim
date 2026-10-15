// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file sessionsmodel.h
/// \brief Declares the table model of client sessions.
///

#pragma once

#include "data/sessioninfo.h"
#include "itemtablemodel.h"

///
/// \brief Table of client sessions: name, client, user, state, subscriptions and activity.
///
class SessionsModel : public ItemTableModel<SessionInfo>
{
    Q_OBJECT

public:
    ///
    /// \brief Columns exposed by the model.
    ///
    enum Column {
        ColName,
        ColClient,
        ColUser,
        ColState,
        ColSubscriptions,
        ColLastActivity,
        ColCount
    };

    ///
    /// \brief Constructs an empty model.
    /// \param parent Owning QObject.
    ///
    explicit SessionsModel(QObject *parent = nullptr);

    ///
    /// \brief Returns the fixed column count.
    /// \param parent Parent index; non-root parents have no columns.
    /// \return Column count, or 0 for non-root parents.
    ///
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns cell text, the session icon, and the state indicator.
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
    /// \brief Finds the row of a session.
    /// \param sessionId Session node id.
    /// \return Row number, or -1 when the session is not listed.
    ///
    int rowOf(const QString &sessionId) const;

    ///
    /// \brief Returns the state text of a session.
    /// \param active Whether the session is open.
    /// \return "Active" or "Closed".
    ///
    static QString stateText(bool active);
};
