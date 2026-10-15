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

///
/// \brief Builds the panel.
/// \param parent Parent widget.
///
EventLogWidget::EventLogWidget(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::EventLogWidget)
{
    ui->setupUi(this);
    ViewSetup::configureTable(ui->logView, ViewSetup::ColumnSizing::Interactive);    connect(ui->clearButton, &QPushButton::clicked, this, &EventLogWidget::clearLog);
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
    ui->logView->setColumnWidth(LogModel::ColTime, 230);
    ui->logView->setColumnWidth(LogModel::ColLevel, 140);
    connect(model, &LogModel::rowsInserted, this, &EventLogWidget::scrollToBottom);
    connect(model, &LogModel::modelReset, this, &EventLogWidget::scrollToBottom);
    scrollToBottom();
}

///
/// \brief Sets the panel title.
/// \param title Title text.
///
void EventLogWidget::setTitle(const QString &title)
{
    ui->titleLabel->setText(title);
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
