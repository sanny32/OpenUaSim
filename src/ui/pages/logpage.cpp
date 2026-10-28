// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file logpage.cpp
/// \brief Implements the server log page.
///

#include <QMenu>

#include "appicons.h"
#include "formatting.h"
#include "logfilterproxymodel.h"
#include "logmodel.h"
#include "logpage.h"
#include "ui_logpage.h"
#include "viewsetup.h"

///
/// \brief Builds the page.
/// \param parent Parent widget.
///
LogPage::LogPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LogPage)
    , _filter(new LogFilterProxyModel(this))
{
    ui->setupUi(this);
    ViewSetup::splitByStretch(ui->splitter);
    // The indicator is text-high; matching the fields keeps the detail rows evenly spaced.
    ui->levelIndicator->setMinimumHeight(ui->timeEdit->sizeHint().height());
    ui->clearLogButton->setIcon(AppIcons::icon(QStringLiteral("trash")));
    ui->exportLogButton->setIcon(AppIcons::icon(QStringLiteral("download")));
    ui->searchEdit->addAction(AppIcons::icon(QStringLiteral("search")), QLineEdit::LeadingPosition);

    auto *categoryMenu = new QMenu(ui->categoryButton);
    categoryMenu->addActions({ ui->serverAction, ui->securityAction, ui->sessionsAction,
                               ui->dataChangesAction });
    ui->categoryButton->setMenu(categoryMenu);

    ViewSetup::configureTable(ui->logView);
    ui->logView->setModel(_filter);

    for (QCheckBox *check : { ui->infoCheck, ui->warningCheck, ui->errorCheck })
        connect(check, &QCheckBox::toggled, this, &LogPage::applyFilters);
    for (QAction *action : categoryMenu->actions())
        connect(action, &QAction::toggled, this, &LogPage::applyFilters);
    connect(ui->searchEdit, &QLineEdit::textChanged, _filter, &LogFilterProxyModel::setFilterFixedString);
    connect(ui->logView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &LogPage::showEntry);
    connect(ui->clearLogButton, &QToolButton::clicked, this, &LogPage::clearLog);
    connect(ui->exportLogButton, &QToolButton::clicked, this, &LogPage::requestExport);
    connect(_filter, &LogFilterProxyModel::rowsInserted, this, &LogPage::scrollIfEnabled);
    connect(_filter, &LogFilterProxyModel::modelReset, this, &LogPage::refreshDetails);
}

///
/// \brief Destroys the page.
///
LogPage::~LogPage()
{
    delete ui;
}

///
/// \brief Shows a log model; the page does not take ownership.
/// \param model Log to show.
///
void LogPage::setLogModel(LogModel *model)
{
    _model = model;
    _filter->setSourceModel(model);
    ui->logView->setColumnWidth(LogModel::ColTime, 170);
    ui->logView->setColumnWidth(LogModel::ColLevel, 110);
    ui->logView->setColumnWidth(LogModel::ColCategory, 105);
    refreshDetails();
}

///
/// \brief Returns the filter between the log model and the table.
/// \return Level, category and search filter.
///
const LogFilterProxyModel *LogPage::filterModel() const
{
    return _filter;
}

///
/// \brief Retranslates the page when the language changes, keeping the shown entry.
/// \param event Change event.
///
void LogPage::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        showEntry(ui->logView->currentIndex());
        if (_model)
            _model->retranslate();
    }
    QWidget::changeEvent(event);
}

///
/// \brief Applies the level check boxes and the category menu to the filter.
///
void LogPage::applyFilters()
{
    _filter->setLevelVisible(LogEntry::Level::Info, ui->infoCheck->isChecked());
    _filter->setLevelVisible(LogEntry::Level::Warning, ui->warningCheck->isChecked());
    _filter->setLevelVisible(LogEntry::Level::Error, ui->errorCheck->isChecked());
    _filter->setCategoryVisible(LogEntry::Category::Server, ui->serverAction->isChecked());
    _filter->setCategoryVisible(LogEntry::Category::Security, ui->securityAction->isChecked());
    _filter->setCategoryVisible(LogEntry::Category::Session, ui->sessionsAction->isChecked());
    _filter->setCategoryVisible(LogEntry::Category::DataChanges, ui->dataChangesAction->isChecked());
}

///
/// \brief Fills the details panel from the selected entry.
/// \param current Selected table index.
///
void LogPage::showEntry(const QModelIndex &current)
{
    const LogEntry *entry = _filter->entry(current);
    ui->detailsHost->setEnabled(entry != nullptr);
    if (!entry) {
        ui->levelIndicator->setStatus(StatusKind::None, QString());
        for (QLineEdit *edit : { ui->timeEdit, ui->categoryEdit, ui->sourceEdit })
            edit->clear();
        ui->messageEdit->clear();
        return;
    }
    ui->timeEdit->setText(Formatting::preciseDateTime(entry->time));
    ui->levelIndicator->setStatus(LogModel::levelStatus(entry->level), LogModel::levelText(entry->level));
    ui->categoryEdit->setText(LogModel::categoryText(entry->category));
    ui->messageEdit->setPlainText(entry->message);
    ui->sourceEdit->setText(entry->source);
}

///
/// \brief Re-reads the details after a model reset, which drops the current index silently;
///        selects the first entry when there is one.
///
void LogPage::refreshDetails()
{
    if (_filter->rowCount() > 0)
        ViewSetup::selectRow(ui->logView, 0);
    showEntry(ui->logView->currentIndex());
}

///
/// \brief Empties the log.
///
void LogPage::clearLog()
{
    if (_model)
        _model->clear();
}

///
/// \brief Asks to save the entries the level, category and search filters let through.
///
void LogPage::requestExport()
{
    QList<LogEntry> entries;
    entries.reserve(_filter->rowCount());
    for (int row = 0; row < _filter->rowCount(); ++row)
        entries.append(*_filter->entry(_filter->index(row, 0)));
    emit exportRequested(entries);
}

///
/// \brief Keeps the newest entry in view while auto-scroll is on.
///
void LogPage::scrollIfEnabled()
{
    if (ui->autoScrollCheck->isChecked())
        ui->logView->scrollToBottom();
}
