// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file statuspage.h
/// \brief Declares the server status page.
///

#pragma once

#include <QWidget>

#include "data/serverstatus.h"

namespace Ui {
class StatusPage;
}

///
/// \brief Page summarising the server state and its key figures.
///
class StatusPage : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the page.
    /// \param parent Parent widget.
    ///
    explicit StatusPage(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the page.
    ///
    ~StatusPage() override;

    ///
    /// \brief Shows a server summary.
    /// \param status Server summary.
    ///
    void setStatus(const ServerStatus &status);

protected:
    void changeEvent(QEvent *event) override;

private:
    Ui::StatusPage *ui;
    ServerStatus _status;
};
