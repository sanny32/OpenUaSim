// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file sessionspage.h
/// \brief Declares the client sessions page.
///

#pragma once

#include <QList>
#include <QWidget>

#include "data/sessioninfo.h"

namespace Ui {
class SessionsPage;
}

class SessionsModel;
class QSortFilterProxyModel;

///
/// \brief Page listing active and closed client sessions and the properties of a session.
///
class SessionsPage : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the page.
    /// \param parent Parent widget.
    ///
    explicit SessionsPage(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the page.
    ///
    ~SessionsPage() override;

    ///
    /// \brief Shows client sessions.
    /// \param sessions Active and closed sessions.
    ///
    void setSessions(const QList<SessionInfo> &sessions);

    ///
    /// \brief Returns the sessions table model.
    /// \return Model of the listed sessions.
    ///
    const SessionsModel *sessionsModel() const;

signals:
    ///
    /// \brief Asks to show the subscriptions of a session.
    /// \param session Selected session; it holds at least one subscription.
    ///
    void subscriptionsRequested(const SessionInfo &session);

protected:
    void changeEvent(QEvent *event) override;

private:
    void applyStateFilter(int filter);
    void showSession(const QModelIndex &current);
    const SessionInfo *sessionAt(const QModelIndex &index) const;
    void requestSubscriptions();
    void showSessionDetails(const SessionInfo &session);

    Ui::SessionsPage *ui;
    SessionsModel *_sessionsModel;
    QSortFilterProxyModel *_stateModel;
    QSortFilterProxyModel *_searchModel;
};
