// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file mainwindow.cpp
/// \brief Implements the main application window.
///

#include <QActionGroup>
#include <QMenu>

#include "appicons.h"
#include "apptheme.h"
#include "data/serversnapshot.h"
#include "logmodel.h"
#include "mainstatusbarwidget.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "viewsetup.h"

///
/// \brief Builds the window.
/// \param theme Theme manager driven by the theme menu; null hides the theme button.
/// \param parent Parent widget.
///
MainWindow::MainWindow(AppTheme *theme, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , _statusWidget(new MainStatusBarWidget(this))
    , _logModel(new LogModel(this))
    , _theme(theme)
{
    ui->setupUi(this);
    setWindowTitle(QStringLiteral(APP_NAME));
    ui->statusBar->addWidget(_statusWidget, 1);
    ui->serverIndicator->setColoredText(true);

    setupIcons();
    setupThemeMenu();

    ui->contentSplitter->setSizes({ 640, 210 });
    ui->eventLog->setLogModel(_logModel);
    ui->logPage->setLogModel(_logModel);

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::handleTabChange);
    connect(_logModel, &LogModel::modelReset, this, &MainWindow::updateStatusBarNote);
    connect(_logModel, &LogModel::rowsInserted, this, &MainWindow::updateStatusBarNote);
    handleTabChange(ui->tabWidget->currentIndex());
}

///
/// \brief Destroys the window.
///
MainWindow::~MainWindow()
{
    delete ui;
}

///
/// \brief Shows a server state on every page.
/// \param snapshot Server state to show.
///
void MainWindow::setSnapshot(const ServerSnapshot &snapshot)
{
    _status = snapshot.status;

    ui->endpointEdit->setText(_status.primaryEndpoint);
    ui->serverIndicator->setStatus(_status.running ? StatusKind::Good : StatusKind::Inactive,
                                   _status.running ? tr("Running") : tr("Stopped"));
    ui->startButton->setEnabled(!_status.running);
    ui->stopButton->setEnabled(_status.running);
    _statusWidget->setServerStatus(_status);

    ui->statusPage->setStatus(_status);
    ui->objectsPage->setAddressSpace(snapshot.addressSpace, snapshot.variables);
    ui->typesPage->setTypes(snapshot.typeTree, snapshot.types);
    ui->namespacesPage->setNamespaces(snapshot.namespaces);
    ui->endpointsPage->setEndpoints(snapshot.endpoints);
    ui->certificatesPage->setCertificates(snapshot.certificates);
    ui->sessionsPage->setSessions(snapshot.sessions);
    _logModel->setItems(snapshot.log);
    updateStatusBarNote();
}

///
/// \brief Returns the log shared by the event log panel and the log page.
/// \return Server log model.
///
LogModel *MainWindow::logModel() const
{
    return _logModel;
}

///
/// \brief Gives the toolbar buttons their icons.
///
void MainWindow::setupIcons()
{
    ui->startButton->setIcon(AppIcons::icon(QStringLiteral("play")));
    ui->stopButton->setIcon(AppIcons::icon(QStringLiteral("stop")));
    ui->addNodeButton->setIcon(AppIcons::icon(QStringLiteral("add-circle")));
    ui->importModelButton->setIcon(AppIcons::icon(QStringLiteral("import")));
    ui->saveButton->setIcon(AppIcons::icon(QStringLiteral("save")));
    ui->themeButton->setIcon(AppIcons::icon(QStringLiteral("theme")));
    ViewSetup::bindCopyButton(ui->copyEndpointButton, ui->endpointEdit);
}

///
/// \brief Builds the System/Light/Dark menu of the theme button.
///
void MainWindow::setupThemeMenu()
{
    if (!_theme) {
        ui->themeButton->hide();
        return;
    }

    auto *group = new QActionGroup(this);
    auto *menu = new QMenu(ui->themeButton);
    for (QAction *action : { ui->actionThemeSystem, ui->actionThemeLight, ui->actionThemeDark }) {
        group->addAction(action);
        menu->addAction(action);
    }
    ui->themeButton->setMenu(menu);
    syncThemeActions();
    connect(group, &QActionGroup::triggered, this, &MainWindow::selectTheme);
}

///
/// \brief Checks the menu action matching the theme preference.
///
void MainWindow::syncThemeActions()
{
    switch (_theme->mode()) {
    case AppTheme::Mode::System: ui->actionThemeSystem->setChecked(true); break;
    case AppTheme::Mode::Light:  ui->actionThemeLight->setChecked(true); break;
    case AppTheme::Mode::Dark:   ui->actionThemeDark->setChecked(true); break;
    }
}

///
/// \brief Applies the theme picked from the menu.
/// \param action Triggered menu action.
///
void MainWindow::selectTheme(QAction *action)
{
    if (action == ui->actionThemeLight)
        _theme->setMode(AppTheme::Mode::Light);
    else if (action == ui->actionThemeDark)
        _theme->setMode(AppTheme::Mode::Dark);
    else
        _theme->setMode(AppTheme::Mode::System);
}

///
/// \brief Adapts the event log panel and the status bar note to the shown page.
/// \param index Index of the shown tab.
///
void MainWindow::handleTabChange(int index)
{
    const QWidget *page = ui->tabWidget->widget(index);
    ui->eventLog->setVisible(page != ui->logPage);
    ui->eventLog->setTitle(page == ui->statusPage ? tr("Recent Activity") : tr("Event Log"));
    updateStatusBarNote();
}

///
/// \brief Shows the log size on the log page and the node count elsewhere.
///
void MainWindow::updateStatusBarNote()
{
    if (ui->tabWidget->currentWidget() == ui->logPage)
        _statusWidget->setTrailingText(tr("Log entries: %1").arg(_logModel->rowCount()));
    else
        _statusWidget->setTrailingText(tr("Nodes: %1").arg(_status.totalNodes));
}
