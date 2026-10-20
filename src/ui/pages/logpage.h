// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file logpage.h
/// \brief Declares the server log page.
///

#pragma once

#include <QList>
#include <QWidget>

#include "data/logentry.h"

namespace Ui {
class LogPage;
}

class LogFilterProxyModel;
class LogModel;

///
/// \brief Page showing the filtered server log and the details of an entry.
///
class LogPage : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the page.
    /// \param parent Parent widget.
    ///
    explicit LogPage(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the page.
    ///
    ~LogPage() override;

    ///
    /// \brief Shows a log model; the page does not take ownership.
    /// \param model Log to show.
    ///
    void setLogModel(LogModel *model);

    ///
    /// \brief Returns the filter between the log model and the table.
    /// \return Level and category filter.
    ///
    const LogFilterProxyModel *filterModel() const;

signals:
    ///
    /// \brief Asks to save the entries the filter shows.
    /// \param entries Visible entries, oldest first.
    ///
    void exportRequested(const QList<LogEntry> &entries);

protected:
    void changeEvent(QEvent *event) override;

private:
    void applyFilters();
    void showEntry(const QModelIndex &current);
    void refreshDetails();
    void clearLog();
    void requestExport();
    void scrollIfEnabled();

    Ui::LogPage *ui;
    LogModel *_model = nullptr;
    LogFilterProxyModel *_filter;
};
