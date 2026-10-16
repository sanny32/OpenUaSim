// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file eventlogwidget.cpp
/// \brief Implements the compact event log panel shown below the pages.
///

#include <QHeaderView>

#include "eventlogwidget.h"
#include "logmodel.h"
#include "ui_eventlogwidget.h"
#include "viewsetup.h"

namespace {

constexpr int kCompactRowHeight = 24;

}

///
/// \brief Builds the panel.
/// \param parent Parent widget.
///
EventLogWidget::EventLogWidget(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::EventLogWidget)
{
    ui->setupUi(this);
    ViewSetup::configureTable(ui->logView);
    // A short strip under the pages: compact rows fit more entries than the delegate's default.
    ui->logView->verticalHeader()->setMinimumSectionSize(kCompactRowHeight);
    ui->logView->verticalHeader()->setDefaultSectionSize(kCompactRowHeight);
    connect(ui->clearButton, &QPushButton::clicked, this, &EventLogWidget::clearLog);
}

///
/// \brief Destroys the panel.
///
EventLogWidget::~EventLogWidget()
{
    delete ui;
}

///
/// \brief Shows a log model; the panel does not take ownership.
/// \param model Log to show.
///
void EventLogWidget::setLogModel(LogModel *model)
{
    _model = model;
    ui->logView->setModel(model);
    ui->logView->hideColumn(LogModel::ColCategory);
    ui->logView->setColumnWidth(LogModel::ColTime, 170);
    ui->logView->setColumnWidth(LogModel::ColLevel, 110);
    connect(model, &LogModel::rowsInserted, this, &EventLogWidget::scrollToBottom);
    connect(model, &LogModel::modelReset, this, &EventLogWidget::scrollToBottom);
    scrollToBottom();
}

///
/// \brief Returns the panel title.
/// \return Title text.
///
QString EventLogWidget::title() const
{
    return ui->titleLabel->text();
}

///
/// \brief Rounds a panel height down so the log shows whole rows only.
/// \param height Height the panel could take.
/// \return Largest height not above \a height that shows whole rows, at least one row.
///
int EventLogWidget::heightForWholeRows(int height) const
{
    const int chrome = this->height() - ui->logView->viewport()->height();
    const int rowHeight = ui->logView->verticalHeader()->defaultSectionSize();
    const int rows = qMax(1, (height - chrome) / rowHeight);
    return chrome + rows * rowHeight;
}

///
/// \brief Empties the shown log.
///
void EventLogWidget::clearLog()
{
    if (_model)
        _model->clear();
}

///
/// \brief Keeps the newest entry in view.
///
void EventLogWidget::scrollToBottom()
{
    ui->logView->scrollToBottom();
}
