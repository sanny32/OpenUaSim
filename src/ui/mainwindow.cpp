// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file mainwindow.cpp
/// \brief Implements the main application window.
///

#include <QActionGroup>
#include <QApplication>
#include <QMessageBox>
#include <QTimer>

#include "appicons.h"
#include "apptheme.h"
#include "data/serversnapshot.h"
#include "logmodel.h"
#include "mainstatusbarwidget.h"
#include "mainwindow.h"
#include "server/servercontroller.h"
#include "ui_mainwindow.h"
#include "viewsetup.h"

namespace {

constexpr double kEventLogShare = 0.28;
constexpr int kPageShortcutCount = 9;
constexpr int kStatusRefreshMs = 1000;

}

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
    , _statusTimer(new QTimer(this))
{
    ui->setupUi(this);
    setWindowTitle(QStringLiteral(APP_NAME));
    ui->statusBar->addWidget(_statusWidget, 1);

    setupIcons();
    setupActions();
    setupViewMenu();
    setupThemeMenu();

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
    applyStatus(snapshot.status);
    ui->objectsPage->setAddressSpace(snapshot.addressSpace, snapshot.variables);
    ui->typesPage->setTypes(snapshot.typeTree, snapshot.types);
    ui->namespacesPage->setNamespaces(snapshot.namespaces);
    ui->endpointsPage->setEndpoints(snapshot.endpoints);
    ui->certificatesPage->setCertificates(snapshot.certificates);
    ui->sessionsPage->setSessions(snapshot.sessions);
    _logModel->setItems(snapshot.log);
}

///
/// \brief Binds the window to the server it controls and shows its state.
/// \param controller Controller serving the project; must outlive the window.
///
void MainWindow::setController(ServerController *controller)
{
    _controller = controller;
    connect(ui->actionStartServer, &QAction::triggered, controller, &ServerController::start);
    connect(ui->actionStopServer, &QAction::triggered, controller, &ServerController::stop);
    connect(ui->actionRestartServer, &QAction::triggered, controller, &ServerController::restart);

    connect(controller, &ServerController::projectChanged, this, &MainWindow::reloadSnapshot);
    connect(controller, &ServerController::runningChanged, this, &MainWindow::refreshStatus);
    connect(controller, &ServerController::sessionsChanged, this, &MainWindow::refreshSessions);
    connect(controller, &ServerController::valuesChanged, ui->objectsPage, &ObjectsPage::updateValues);
    connect(controller, &ServerController::logAppended, this, &MainWindow::appendLog);
    connect(controller, &ServerController::startFailed, this, &MainWindow::showStartFailure);

    _statusTimer->setInterval(kStatusRefreshMs);
    connect(_statusTimer, &QTimer::timeout, this, &MainWindow::refreshStatus);
    _statusTimer->start();
    reloadSnapshot();
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
/// \brief Splits the content between the pages and the event log on the first show, without a cut-off log row.
/// \param event Show event.
///
void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    if (_contentSplitApplied)
        return;
    _contentSplitApplied = true;

    // Sizes are only known once the window is laid out; the pages' minimum height still wins.
    const QList<int> sizes = ui->contentSplitter->sizes();
    const int total = sizes.value(0) + sizes.value(1);
    const int log = ui->eventLog->heightForWholeRows(qRound(total * kEventLogShare));
    ui->contentSplitter->setSizes({ total - log, log });
}

///
/// \brief Gives the toolbar buttons and the menu actions their icons.
///
void MainWindow::setupIcons()
{
    const QIcon play = AppIcons::icon(QStringLiteral("play"));
    const QIcon stop = AppIcons::icon(QStringLiteral("stop"));
    const QIcon addNode = AppIcons::icon(QStringLiteral("add-circle"));
    const QIcon importModel = AppIcons::icon(QStringLiteral("import"));
    const QIcon save = AppIcons::icon(QStringLiteral("save"));

    ui->startButton->setIcon(play);
    ui->actionStartServer->setIcon(play);
    ui->stopButton->setIcon(stop);
    ui->actionStopServer->setIcon(stop);
    ui->addNodeButton->setIcon(addNode);
    ui->actionAddNode->setIcon(addNode);
    ui->importModelButton->setIcon(importModel);
    ui->actionImportModel->setIcon(importModel);
    ui->saveButton->setIcon(save);
    ui->actionSaveProject->setIcon(save);
    ui->actionExportModel->setIcon(AppIcons::icon(QStringLiteral("upload")));
    ui->themeButton->setIcon(AppIcons::icon(QStringLiteral("theme")));
    ViewSetup::addCopyAction(ui->endpointEdit);
}

///
/// \brief Routes the toolbar buttons through the menu actions and wires the window-level actions.
///
void MainWindow::setupActions()
{
    connect(ui->startButton, &QPushButton::clicked, ui->actionStartServer, &QAction::trigger);
    connect(ui->stopButton, &QPushButton::clicked, ui->actionStopServer, &QAction::trigger);
    connect(ui->addNodeButton, &QPushButton::clicked, ui->actionAddNode, &QAction::trigger);
    connect(ui->importModelButton, &QPushButton::clicked, ui->actionImportModel, &QAction::trigger);
    connect(ui->saveButton, &QPushButton::clicked, ui->actionSaveProject, &QAction::trigger);
    connect(ui->actionStartServer, &QAction::enabledChanged, ui->startButton, &QWidget::setEnabled);
    connect(ui->actionStopServer, &QAction::enabledChanged, ui->stopButton, &QWidget::setEnabled);

    connect(ui->actionEventLog, &QAction::toggled, this, &MainWindow::updateEventLogVisibility);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAbout);
    connect(ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
}

///
/// \brief Lists the pages at the top of the View menu, with Ctrl+digit shortcuts.
///
void MainWindow::setupViewMenu()
{
    _pageActions = new QActionGroup(this);
    QAction *before = ui->menuView->actions().value(0);
    for (int index = 0; index < ui->tabWidget->count(); ++index) {
        auto *action = new QAction(ui->tabWidget->tabText(index), _pageActions);
        action->setCheckable(true);
        action->setData(index);
        if (index < kPageShortcutCount)
            action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key(Qt::Key_1 + index)));
        ui->menuView->insertAction(before, action);
    }
    connect(_pageActions, &QActionGroup::triggered, this, &MainWindow::selectPage);
}

///
/// \brief Shows the page a View menu action stands for.
/// \param action Triggered page action.
///
void MainWindow::selectPage(QAction *action)
{
    ui->tabWidget->setCurrentIndex(action->data().toInt());
}

///
/// \brief Shows the application name, version and purpose.
///
void MainWindow::showAbout()
{
    QMessageBox::about(this, tr("About %1").arg(QStringLiteral(APP_NAME)),
                       tr("<h3>%1 %2</h3><p>OPC UA simulation server.</p>")
                           .arg(QStringLiteral(APP_NAME), QStringLiteral(APP_VERSION)));
}

///
/// \brief Shares the System/Light/Dark menu between View > Theme and the theme button.
///
void MainWindow::setupThemeMenu()
{
    if (!_theme) {
        ui->themeButton->hide();
        ui->menuTheme->menuAction()->setVisible(false);
        return;
    }

    auto *group = new QActionGroup(this);
    for (QAction *action : { ui->actionThemeSystem, ui->actionThemeLight, ui->actionThemeDark })
        group->addAction(action);
    ui->themeButton->setMenu(ui->menuTheme);
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
/// \brief Checks the page in the View menu and adapts the event log panel and the status bar note.
/// \param index Index of the shown tab.
///
void MainWindow::handleTabChange(int index)
{
    if (QAction *action = _pageActions->actions().value(index))
        action->setChecked(true);
    updateEventLogVisibility();
    updateStatusBarNote();
}

///
/// \brief Shows the event log panel when it is switched on, except on the log page that already lists the log.
///
void MainWindow::updateEventLogVisibility()
{
    const bool onLogPage = ui->tabWidget->currentWidget() == ui->logPage;
    ui->actionEventLog->setEnabled(!onLogPage);
    ui->eventLog->setVisible(ui->actionEventLog->isChecked() && !onLogPage);
}

///
/// \brief Shows the log size on the log page and clears the note elsewhere.
///
void MainWindow::updateStatusBarNote()
{
    const bool onLogPage = ui->tabWidget->currentWidget() == ui->logPage;
    _statusWidget->setTrailingText(onLogPage ? tr("Log entries: %1").arg(_logModel->rowCount()) : QString());
}

///
/// \brief Shows a server summary in the toolbar, the status page and the status bar.
/// \param status Server summary.
///
void MainWindow::applyStatus(const ServerStatus &status)
{
    ui->endpointEdit->setText(status.primaryEndpoint);
    ui->actionStartServer->setEnabled(!status.running);
    ui->actionStopServer->setEnabled(status.running);
    ui->actionRestartServer->setEnabled(status.running);
    _statusWidget->setServerStatus(status);
    ui->statusPage->setStatus(status);
}

///
/// \brief Shows the controller's current server summary, e.g. to advance the uptime.
///
void MainWindow::refreshStatus()
{
    if (_controller)
        applyStatus(_controller->status());
}

///
/// \brief Shows the controller's whole state on every page.
///
void MainWindow::reloadSnapshot()
{
    if (_controller)
        setSnapshot(_controller->snapshot());
}

///
/// \brief Appends a server log entry to the log page and the event log.
/// \param entry New entry.
///
void MainWindow::appendLog(const LogEntry &entry)
{
    _logModel->appendItems({ entry }, ServerController::kLogLimit);
}

///
/// \brief Shows the current sessions and the connected client count.
///
void MainWindow::refreshSessions()
{
    if (!_controller)
        return;
    ui->sessionsPage->setSessions(_controller->sessions());
    refreshStatus();
}

///
/// \brief Tells the user why the server did not start, without blocking the event loop.
/// \param reason Explanation from the server.
///
void MainWindow::showStartFailure(const QString &reason)
{
    auto *box = new QMessageBox(QMessageBox::Warning, tr("Server not started"), reason, QMessageBox::Ok, this);
    box->setAttribute(Qt::WA_DeleteOnClose);
    box->open();
}
