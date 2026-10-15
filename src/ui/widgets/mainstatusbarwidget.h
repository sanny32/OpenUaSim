// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file mainstatusbarwidget.h
/// \brief Declares the content of the main window status bar.
///

#pragma once

#include <QWidget>

namespace Ui {
class MainStatusBarWidget;
}

struct ServerStatus;

///
/// \brief Status bar content: server state, endpoint, client count and a trailing note.
///
class MainStatusBarWidget : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the status bar content.
    /// \param parent Parent widget.
    ///
    explicit MainStatusBarWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the widget.
    ///
    ~MainStatusBarWidget() override;

    ///
    /// \brief Shows the server state, its primary endpoint and the client count.
    /// \param status Server summary.
    ///
    void setServerStatus(const ServerStatus &status);

    ///
    /// \brief Sets the right-aligned note, e.g. the node or log entry count.
    /// \param text Note text.
    ///
    void setTrailingText(const QString &text);

    ///
    /// \brief Returns the right-aligned note.
    /// \return Note text.
    ///
    QString trailingText() const;

private:
    Ui::MainStatusBarWidget *ui;
};
