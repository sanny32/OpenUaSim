// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file endpointspage.h
/// \brief Declares the endpoints page.
///

#pragma once

#include <QList>
#include <QWidget>

#include "data/endpointinfo.h"

namespace Ui {
class EndpointsPage;
}

class EndpointsModel;
class QSortFilterProxyModel;

///
/// \brief Page listing the server endpoints and the settings of an endpoint.
///
class EndpointsPage : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the page.
    /// \param parent Parent widget.
    ///
    explicit EndpointsPage(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the page.
    ///
    ~EndpointsPage() override;

    ///
    /// \brief Shows the configured endpoints.
    /// \param endpoints Endpoints to list.
    ///
    void setEndpoints(const QList<EndpointInfo> &endpoints);

    ///
    /// \brief Returns the endpoints table model.
    /// \return Model of the configured endpoints.
    ///
    const EndpointsModel *endpointsModel() const;

private:
    void showEndpoint(const QModelIndex &current);
    void updateCredentialFields();

    Ui::EndpointsPage *ui;
    EndpointsModel *_endpointsModel;
    QSortFilterProxyModel *_searchModel;
};
