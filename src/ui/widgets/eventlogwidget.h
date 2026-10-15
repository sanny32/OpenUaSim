// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file eventlogwidget.h
/// \brief Declares the compact event log panel shown below the pages.
///

#pragma once

#include <QFrame>

namespace Ui {
class EventLogWidget;
}

class LogModel;

///
/// \brief Panel listing the server log as time, level and message.
///
class EventLogWidget : public QFrame
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the panel.
    /// \param parent Parent widget.
    ///
    explicit EventLogWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the panel.
    ///
    ~EventLogWidget() override;

    ///
    /// \brief Shows a log model; the panel does not take ownership.
    /// \param model Log to show.
    ///
    void setLogModel(LogModel *model);

    ///
    /// \brief Sets the panel title.
    /// \param title Title text.
    ///
    void setTitle(const QString &title);

    ///
    /// \brief Returns the panel title.
    /// \return Title text.
    ///
    QString title() const;

private:
    void clearLog();
    void scrollToBottom();

    Ui::EventLogWidget *ui;
    LogModel *_model = nullptr;
};
