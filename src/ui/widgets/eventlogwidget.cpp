// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file eventlogwidget.cpp
/// \brief Implements the compact event log panel shown below the pages.
///

#include <QClipboard>
#include <QGuiApplication>
#include <QHeaderView>
#include <QScrollBar>

#include "appicons.h"
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
    ui->clearButton->setIcon(AppIcons::icon(QStringLiteral("trash")));
    ui->closeButton->setIcon(AppIcons::icon(QStringLiteral("x")));
    ui->actionCopyEntry->setIcon(AppIcons::icon(QStringLiteral("copy")));
    ui->logView->addAction(ui->actionCopyEntry);
    ViewSetup::configureTable(ui->logView);
    // A short strip under the pages: compact rows fit more entries than the delegate's default.
    ui->logView->verticalHeader()->setMinimumSectionSize(kCompactRowHeight);
    ui->logView->verticalHeader()->setDefaultSectionSize(kCompactRowHeight);
    connect(ui->clearButton, &QToolButton::clicked, this, &EventLogWidget::clearLog);
    connect(ui->closeButton, &QToolButton::clicked, this, &EventLogWidget::closeRequested);
    connect(ui->actionCopyEntry, &QAction::triggered, this, &EventLogWidget::copyEntry);
}

///
/// \brief Destroys the panel.
///
EventLogWidget::~EventLogWidget()
{
    delete ui;
}

///
/// \brief Retranslates the panel when the language changes; the log page retranslates the shared model.
/// \param event Change event.
///
void EventLogWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        ui->retranslateUi(this);
    QFrame::changeEvent(event);
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
    connect(model, &LogModel::rowsAboutToBeInserted, this, &EventLogWidget::rememberScrollPosition);
    connect(model, &LogModel::rowsInserted, this, &EventLogWidget::followNewRows);
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
/// \brief Puts the time, level and message of the selected entry on the clipboard, tab-separated.
///
void EventLogWidget::copyEntry()
{
    const QModelIndex current = ui->logView->currentIndex();
    if (!current.isValid() || !ui->logView->selectionModel()->isRowSelected(current.row()))
        return;
    const QStringList fields = {
        current.siblingAtColumn(LogModel::ColTime).data().toString(),
        current.siblingAtColumn(LogModel::ColLevel).data().toString(),
        current.siblingAtColumn(LogModel::ColMessage).data().toString(),
    };
    QGuiApplication::clipboard()->setText(fields.join(QLatin1Char('\t')));
}

///
/// \brief Notes whether the newest entry is in view before rows arrive.
///
void EventLogWidget::rememberScrollPosition()
{
    const QScrollBar *bar = ui->logView->verticalScrollBar();
    _followNewRows = bar->value() >= bar->maximum();
}

///
/// \brief Scrolls to arrived rows unless the user scrolled up to read older entries.
///
void EventLogWidget::followNewRows()
{
    if (_followNewRows)
        scrollToBottom();
}

///
/// \brief Brings the newest entry into view.
///
void EventLogWidget::scrollToBottom()
{
    ui->logView->scrollToBottom();
}
