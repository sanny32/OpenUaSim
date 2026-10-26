// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file mainwindow.cpp
/// \brief Implements the main application window.
///

#include <memory>

#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QPushButton>
#include <QTimer>

#include "addnodedialog.h"
#include "alarmdialog.h"
#include "appicons.h"
#include "applanguage.h"
#include "apptheme.h"
#include "certificateactions.h"
#include "messagedialog.h"
#include "data/serversnapshot.h"
#include "logexport.h"
#include "logmodel.h"
#include "mainstatusbarwidget.h"
#include "mainwindow.h"
#include "modelactions.h"
#include "elidedlabel.h"
#include "project/projectdocument.h"
#include "project/projectfile.h"
#include "project/projectnodes.h"
#include "server/servercontroller.h"
#include "serversettingsdialog.h"
#include "subscriptionsdialog.h"
#include "ui_mainwindow.h"
#include "usersdialog.h"
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
    , _restartNote(new ElidedLabel(tr("Restart the server to apply the changes"), this))
{
    ui->setupUi(this);
    setWindowTitle(QStringLiteral(APP_NAME));
    ui->statusBar->addWidget(_statusWidget, 1);
    ui->statusBar->addPermanentWidget(_restartNote);
    _restartNote->hide();

    setupIcons();
    setupActions();
    setupViewMenu();
    setupThemeMenu();
    setupLanguageMenu();

    ui->eventLog->setLogModel(_logModel);
    ui->logPage->setLogModel(_logModel);

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::handleTabChange);
    connect(_logModel, &LogModel::modelReset, this, &MainWindow::updateStatusBarNote);
    connect(_logModel, &LogModel::rowsInserted, this, &MainWindow::updateStatusBarNote);
    connect(ui->logPage, &LogPage::exportRequested, this, &MainWindow::exportLog);
    connect(ui->sessionsPage, &SessionsPage::subscriptionsRequested, this, &MainWindow::showSubscriptions);
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
    ui->typesPage->setTypes(snapshot.types);
    ui->namespacesPage->setNamespaces(snapshot.namespaces);
    _types = snapshot.types;
    _namespaces.clear();
    for (const NamespaceInfo &info : snapshot.namespaces)
        _namespaces.append(info.uri);
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
    connect(controller, &ServerController::startWarning, this, &MainWindow::showStartWarning);
    connect(controller, &ServerController::variableChanged, ui->objectsPage, &ObjectsPage::updateVariable);
    connect(controller, &ServerController::variableQualityChanged, ui->objectsPage, &ObjectsPage::updateQuality);
    connect(ui->objectsPage, &ObjectsPage::variableChangeRequested, this, &MainWindow::applyVariableChange);
    connect(ui->objectsPage, &ObjectsPage::alarmEditRequested, this, &MainWindow::showAlarmDialog);
    connect(ui->objectsPage, &ObjectsPage::currentNodeChanged, this, &MainWindow::updateNodeActions);
    connect(ui->actionAddNode, &QAction::triggered, this, &MainWindow::showAddNodeDialog);
    connect(ui->actionRenameNode, &QAction::triggered, this, &MainWindow::showRenameDialog);
    connect(ui->actionDuplicateNode, &QAction::triggered, this, &MainWindow::duplicateNode);
    connect(ui->actionCopyNode, &QAction::triggered, this, &MainWindow::copyNode);
    connect(ui->actionPasteNode, &QAction::triggered, this, &MainWindow::pasteNode);
    connect(ui->actionDeleteNode, &QAction::triggered, this, &MainWindow::confirmDeleteNode);
    connect(ui->actionServerSettings, &QAction::triggered, this, &MainWindow::showServerSettingsDialog);
    connect(ui->actionUserAccounts, &QAction::triggered, this, &MainWindow::showUsersDialog);
    _certificateActions = new CertificateActions(*controller, ui->certificatesPage, this);
    _modelActions = new ModelActions(*controller, ui->namespacesPage, this);
    connect(ui->actionImportModel, &QAction::triggered, _modelActions, &ModelActions::importModel);
    connect(ui->actionExportModel, &QAction::triggered, _modelActions, &ModelActions::exportModel);
    connect(ui->endpointsPage, &EndpointsPage::endpointsChangeRequested, this, &MainWindow::applyEndpoints);
    connect(ui->endpointsPage, &EndpointsPage::usersEditRequested, this, &MainWindow::showUsersDialog);
    connect(controller, &ServerController::endpointsChanged, this, &MainWindow::reloadEndpoints);
    connect(controller, &ServerController::restartRequiredChanged, this, &MainWindow::updateRestartNote);

    _statusTimer->setInterval(kStatusRefreshMs);
    connect(_statusTimer, &QTimer::timeout, this, &MainWindow::refreshStatus);
    _statusTimer->start();
    reloadSnapshot();
    updateNodeActions();
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
/// \brief Returns the actions of the Certificates page.
/// \return Actions created by setController(), or null before.
///
CertificateActions *MainWindow::certificateActions() const
{
    return _certificateActions;
}

///
/// \brief Returns the actions on NodeSet2 models.
/// \return Actions created by setController(), or null before.
///
ModelActions *MainWindow::modelActions() const
{
    return _modelActions;
}

///
/// \brief Binds the File menu to the project document and shows its name in the title.
/// \param document Document of the served project; must outlive the window.
///
void MainWindow::setDocument(ProjectDocument *document)
{
    _document = document;
    connect(ui->actionNewProject, &QAction::triggered, this, &MainWindow::newProject);
    connect(ui->actionOpenProject, &QAction::triggered, this, &MainWindow::openProject);
    connect(ui->actionSaveProject, &QAction::triggered, this, &MainWindow::saveProject);
    connect(ui->actionSaveProjectAs, &QAction::triggered, this, &MainWindow::saveProjectAs);
    connect(document, &ProjectDocument::fileNameChanged, this, &MainWindow::updateWindowTitle);
    connect(document, &ProjectDocument::modifiedChanged, this, &MainWindow::updateWindowTitle);
    updateWindowTitle();
}

///
/// \brief Serves a project file, reporting a file that cannot be read.
/// \param fileName File to open.
/// \return True when the file was opened.
///
bool MainWindow::openProjectFile(const QString &fileName)
{
    QString error;
    if (_document && _document->open(fileName, &error))
        return true;
    showError(tr("Project not opened"), error);
    return false;
}

///
/// \brief Saves the project to a file, reporting a file that cannot be written.
/// \param fileName Target path.
/// \return True when the file was written.
///
bool MainWindow::saveProjectTo(const QString &fileName)
{
    QString error;
    if (_document && _document->saveAs(fileName, &error))
        return true;
    showError(tr("Project not saved"), error);
    return false;
}

///
/// \brief Asks whether unsaved changes may be dropped, offering to save them first.
/// \return True when nothing is unsaved, the changes were saved or the user discards them.
///
bool MainWindow::confirmDiscardChanges()
{
    if (!_document || !_document->isModified())
        return true;
    const std::unique_ptr<MessageDialog> dialog(MessageDialog::confirmation(
        tr("Unsaved Changes"), tr("Save the changes to '%1'?").arg(_document->displayName()),
        tr("Your changes will be lost if you don't save them."), tr("Save"), this));
    dialog->setAlternative(tr("Don't Save"));
    dialog->setActionDefault();
    const int answer = dialog->exec();
    if (answer == QDialog::Accepted)
        return saveProject();
    return answer == MessageDialog::Alternative;
}

///
/// \brief Writes log entries to a CSV file, reporting a file that cannot be written.
/// \param fileName Target path.
/// \param entries Entries to write.
/// \return True when the file was written.
///
bool MainWindow::exportLogTo(const QString &fileName, const QList<LogEntry> &entries)
{
    QString error;
    if (LogExport::write(fileName, entries, &error))
        return true;
    showError(tr("Log not exported"), error);
    return false;
}

///
/// \brief Keeps the window open when the user cancels dropping unsaved changes.
/// \param event Close event.
///
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (confirmDiscardChanges())
        QMainWindow::closeEvent(event);
    else
        event->ignore();
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

    const QList<QAction *> nodeActions = { ui->actionAddNode, ui->actionRenameNode, ui->actionDuplicateNode,
                                           ui->actionCopyNode, ui->actionPasteNode, ui->actionDeleteNode };
    ui->objectsPage->setTreeActions(nodeActions);
    ui->objectsPage->addActions({ ui->actionDuplicateNode, ui->actionCopyNode, ui->actionPasteNode });
    for (QAction *action : nodeActions.mid(1))
        action->setEnabled(false);
    connect(ui->actionEventLog, &QAction::toggled, this, &MainWindow::updateEventLogVisibility);
    connect(ui->eventLog, &EventLogWidget::closeRequested, this, &MainWindow::hideEventLog);
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
    auto *dialog = new MessageDialog(QStyle::SP_MessageBoxInformation, tr("About %1").arg(QStringLiteral(APP_NAME)),
                                     QStringLiteral("%1 %2").arg(QStringLiteral(APP_NAME), QStringLiteral(APP_VERSION)),
                                     tr("OPC UA simulation server."), this);
    dialog->setIcon(windowIcon());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->open();
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
/// \brief Groups the language actions and checks the saved preference.
///
void MainWindow::setupLanguageMenu()
{
    auto *group = new QActionGroup(this);
    const QList<QPair<QAction *, AppLanguage::Language>> actions = {
        { ui->actionLanguageSystem, AppLanguage::Language::System },
        { ui->actionLanguageEnglish, AppLanguage::Language::English },
        { ui->actionLanguageRussian, AppLanguage::Language::Russian },
    };
    const AppLanguage::Language saved = AppLanguage::saved();
    for (const auto &[action, language] : actions) {
        action->setData(int(language));
        action->setChecked(language == saved);
        group->addAction(action);
    }
    connect(group, &QActionGroup::triggered, this, &MainWindow::selectLanguage);
}

///
/// \brief Saves the chosen language and switches the interface to it at once.
/// \param action Triggered language action.
///
void MainWindow::selectLanguage(QAction *action)
{
    const auto language = AppLanguage::Language(action->data().toInt());
    AppLanguage::save(language);
    AppLanguage::apply(*QCoreApplication::instance(), language);
}

///
/// \brief Retranslates the window when the application language changes.
/// \param event Change event.
///
void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        retranslate();
    QMainWindow::changeEvent(event);
}

///
/// \brief Reapplies the translated texts of the window; the pages retranslate themselves.
///
void MainWindow::retranslate()
{
    ui->retranslateUi(this);
    const QList<QAction *> pageActions = _pageActions->actions();
    for (QAction *action : pageActions)
        action->setText(ui->tabWidget->tabText(action->data().toInt()));
    _restartNote->setText(tr("Restart the server to apply the changes"));
    if (_document)
        updateWindowTitle();
    else
        setWindowTitle(QStringLiteral(APP_NAME));
    updateStatusBarNote();
    // The pages retranslate after this window, so texts computed from the server state come last.
    QMetaObject::invokeMethod(this, &MainWindow::reloadSnapshot, Qt::QueuedConnection);
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
/// \brief Switches the event log off through its menu action so the menu stays in sync.
///
void MainWindow::hideEventLog()
{
    ui->actionEventLog->setChecked(false);
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
    // setText() drops the selection, and the status refreshes every second.
    if (ui->endpointEdit->text() != status.primaryEndpoint)
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
    showError(tr("Server not started"), reason);
}

///
/// \brief Tells the user that the server started with less than configured, without blocking the event loop.
/// \param reason Explanation from the server.
///
void MainWindow::showStartWarning(const QString &reason)
{
    showError(tr("Secure endpoints not offered"), reason);
}

///
/// \brief Shows a warning without blocking the event loop.
/// \param title Window title.
/// \param text Message.
///
void MainWindow::showError(const QString &title, const QString &text)
{
    MessageDialog::warning(title, text, this);
}

///
/// \brief Opens the Add Node dialog for the selected folder or object, or for the top level.
///
void MainWindow::showAddNodeDialog()
{
    if (!_controller)
        return;
    const NodeDefinition *selected = targetContainer();
    auto *dialog = new AddNodeDialog(_controller->project().nodes, selected ? selected->nodeId : QString(), this);
    dialog->setTypes(_types, _namespaces);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &QDialog::accepted, this, &MainWindow::addNodeFromDialog);
    dialog->open();
}

///
/// \brief Adds the node described by the accepted Add Node dialog and selects it.
///
void MainWindow::addNodeFromDialog()
{
    const auto *dialog = qobject_cast<const AddNodeDialog *>(sender());
    if (!dialog || !_controller)
        return;
    QString error;
    const QString nodeId = _controller->addNode(dialog->parentId(), dialog->node(), &error);
    if (nodeId.isEmpty()) {
        showError(tr("Node not added"), error);
        return;
    }
    ui->objectsPage->selectNode(nodeId);
}

///
/// \brief Returns the project folder or object that new and pasted nodes go to.
/// \return Selected folder or object, the owner of a selected variable or method, or null for the top level; a node
///         that a type declares is replaced by the project node holding it.
///
const NodeDefinition *MainWindow::targetContainer() const
{
    const QList<NodeDefinition> &nodes = _controller->project().nodes;
    const NodeDefinition *selected = ProjectNodes::find(nodes, ui->objectsPage->currentNodeId());
    if (selected && (selected->kind == NodeDefinition::Kind::Variable || selected->kind == NodeDefinition::Kind::Method))
        selected = ProjectNodes::find(nodes, ui->objectsPage->currentContainerId());
    while (selected && selected->declared)
        selected = ProjectNodes::find(nodes, ProjectNodes::parentId(nodes, selected->nodeId));
    return selected;
}

///
/// \brief Copies the selected project node next to itself and selects the copy.
///
void MainWindow::duplicateNode()
{
    if (!_controller)
        return;
    const QString nodeId = ui->objectsPage->currentNodeId();
    if (ProjectNodes::find(_controller->project().nodes, nodeId))
        copyNodeTo(nodeId, ProjectNodes::parentId(_controller->project().nodes, nodeId));
}

///
/// \brief Remembers the selected project node for Paste Node.
///
void MainWindow::copyNode()
{
    if (!_controller || !ProjectNodes::find(_controller->project().nodes, ui->objectsPage->currentNodeId()))
        return;
    _copiedNodeId = ui->objectsPage->currentNodeId();
    updateNodeActions();
}

///
/// \brief Copies the node remembered by Copy Node into the selected folder or object.
///
void MainWindow::pasteNode()
{
    if (!_controller)
        return;
    const NodeDefinition *target = targetContainer();
    copyNodeTo(_copiedNodeId, target ? target->nodeId : QString());
}

///
/// \brief Copies a node with its children, selecting the copy or reporting why it was refused.
/// \param nodeId Node to copy.
/// \param parentId Folder or object receiving the copy; empty for the top level.
///
void MainWindow::copyNodeTo(const QString &nodeId, const QString &parentId)
{
    QString error;
    const QString copyId = _controller->copyNode(nodeId, parentId, &error);
    if (copyId.isEmpty()) {
        showError(tr("Node not copied"), error);
        return;
    }
    ui->objectsPage->selectNode(copyId);
}

///
/// \brief Asks before deleting the selected project nodes with their children.
///
void MainWindow::confirmDeleteNode()
{
    if (!_controller)
        return;
    const QList<NodeDefinition> &nodes = _controller->project().nodes;
    _pendingDeleteIds.clear();
    const QStringList selected = ui->objectsPage->selectedNodeIds();
    for (const QString &nodeId : selected) {
        if (ProjectNodes::find(nodes, nodeId))
            _pendingDeleteIds.append(nodeId);
    }
    if (_pendingDeleteIds.isEmpty())
        return;
    const QString question = _pendingDeleteIds.size() == 1
        ? tr("Delete '%1' and everything below it?").arg(ProjectNodes::find(nodes, _pendingDeleteIds.first())->browseName)
        : tr("Delete %n nodes and everything below them?", nullptr, int(_pendingDeleteIds.size()));
    auto *dialog = MessageDialog::confirmation(tr("Delete Node"), question, QString(), tr("Delete"), this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &QDialog::accepted, this, &MainWindow::deletePendingNode);
    dialog->open();
}

///
/// \brief Asks for a new browse name of the selected project node.
///
void MainWindow::showRenameDialog()
{
    if (!_controller)
        return;
    const NodeDefinition *node = ProjectNodes::find(_controller->project().nodes, ui->objectsPage->currentNodeId());
    if (!node)
        return;
    _pendingRenameId = node->nodeId;
    auto *dialog = new QInputDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(tr("Rename Node"));
    dialog->setLabelText(tr("New name of '%1':").arg(node->browseName));
    dialog->setTextValue(node->browseName);
    connect(dialog, &QInputDialog::textValueSelected, this, &MainWindow::renamePendingNode);
    dialog->open();
}

///
/// \brief Gives the node of the accepted Rename Node dialog its new name and keeps it selected.
/// \param browseName Entered name; surrounding spaces are dropped.
///
void MainWindow::renamePendingNode(const QString &browseName)
{
    if (!_controller)
        return;
    QString error;
    if (!_controller->renameNode(_pendingRenameId, browseName.trimmed(), &error)) {
        showError(tr("Node not renamed"), error);
        return;
    }
    ui->objectsPage->selectNode(_pendingRenameId);
}

///
/// \brief Shows the server description for editing.
///
void MainWindow::showServerSettingsDialog()
{
    if (!_controller)
        return;
    auto *dialog = new ServerSettingsDialog(_controller->project().server, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &QDialog::accepted, this, &MainWindow::applyServerSettingsFromDialog);
    dialog->open();
}

///
/// \brief Applies the settings of the accepted Server Settings dialog.
///
void MainWindow::applyServerSettingsFromDialog()
{
    const auto *dialog = qobject_cast<const ServerSettingsDialog *>(sender());
    if (!dialog || !_controller)
        return;
    QString error;
    if (!_controller->setServerSettings(dialog->settings(), &error)) {
        showError(tr("Server settings not applied"), error);
        return;
    }
    refreshStatus();
}

///
/// \brief Shows the alarm of a variable for editing.
/// \param variable Variable as stored.
///
void MainWindow::showAlarmDialog(const VariableInfo &variable)
{
    _pendingAlarmVariable = variable;
    auto *dialog = new AlarmDialog(variable, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &QDialog::accepted, this, &MainWindow::applyAlarmFromDialog);
    dialog->open();
}

///
/// \brief Applies the alarm of the accepted Alarm dialog, keeping the variable's other properties.
///
void MainWindow::applyAlarmFromDialog()
{
    const auto *dialog = qobject_cast<const AlarmDialog *>(sender());
    if (!dialog)
        return;
    VariableInfo variable = _pendingAlarmVariable;
    variable.alarm = dialog->alarm();
    // An invalid initial value keeps the stored one instead of resetting the live value.
    variable.initialValue = QVariant();
    applyVariableChange(variable);
}

///
/// \brief Shows the project's user accounts for editing.
///
void MainWindow::showUsersDialog()
{
    if (!_controller)
        return;
    auto *dialog = new UsersDialog(_controller->project().users, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &QDialog::accepted, this, &MainWindow::applyUsersFromDialog);
    dialog->open();
}

///
/// \brief Applies the accounts of the accepted User Accounts dialog.
///
void MainWindow::applyUsersFromDialog()
{
    const auto *dialog = qobject_cast<const UsersDialog *>(sender());
    if (!dialog || !_controller)
        return;
    QString error;
    if (!_controller->setUsers(dialog->users(), &error))
        showError(tr("User accounts not applied"), error);
}

///
/// \brief Deletes the nodes the Delete Node question was confirmed for.
///
void MainWindow::deletePendingNode()
{
    if (_controller)
        _controller->removeNodes(_pendingDeleteIds);
}

///
/// \brief Applies edited variable properties and reports a refused change.
/// \param variable Variable with the edited fields.
///
void MainWindow::applyVariableChange(const VariableInfo &variable)
{
    QString error;
    if (_controller && !_controller->updateVariable(variable, &error))
        showError(tr("Changes not applied"), error);
}

///
/// \brief Stores edited endpoints and reports a refused change.
/// \param endpoints Complete new endpoint list.
///
void MainWindow::applyEndpoints(const QList<EndpointInfo> &endpoints)
{
    QString error;
    if (_controller && !_controller->setEndpoints(endpoints, &error))
        showError(tr("Endpoints not changed"), error);
}

///
/// \brief Shows the controller's endpoints and the endpoint URL they lead to.
///
void MainWindow::reloadEndpoints()
{
    if (!_controller)
        return;
    ui->endpointsPage->setEndpoints(_controller->project().endpoints);
    refreshStatus();
}

///
/// \brief Shows or hides the note that endpoint changes wait for a restart.
/// \param required Whether a restart is pending.
///
void MainWindow::updateRestartNote(bool required)
{
    _restartNote->setVisible(required);
}

///
/// \brief Replaces the project with the default one once unsaved changes are settled.
///
void MainWindow::newProject()
{
    if (_document && confirmDiscardChanges())
        _document->newProject();
}

///
/// \brief Picks and opens a project file once unsaved changes are settled.
///
void MainWindow::openProject()
{
    if (!_document || !confirmDiscardChanges())
        return;
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Open Project"), _document->fileName(),
                                                          ProjectFile::fileFilter());
    if (!fileName.isEmpty())
        openProjectFile(fileName);
}

///
/// \brief Saves to the document's file, asking for one when the project was never saved.
/// \return True when the project was written.
///
bool MainWindow::saveProject()
{
    if (!_document)
        return false;
    if (_document->fileName().isEmpty())
        return saveProjectAs();
    return saveProjectTo(_document->fileName());
}

///
/// \brief Asks for a file name and saves the project there.
/// \return True when the project was written.
///
bool MainWindow::saveProjectAs()
{
    if (!_document)
        return false;
    const QString fileName = QFileDialog::getSaveFileName(this, tr("Save Project As"), _document->fileName(),
                                                          ProjectFile::fileFilter());
    return !fileName.isEmpty() && saveProjectTo(fileName);
}

///
/// \brief Lists the subscriptions of a session in a dialog.
/// \param session Session to describe.
///
void MainWindow::showSubscriptions(const SessionInfo &session)
{
    auto *dialog = new SubscriptionsDialog(session, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->open();
}

///
/// \brief Asks for a file name and writes the visible log entries there.
/// \param entries Entries to write.
///
void MainWindow::exportLog(const QList<LogEntry> &entries)
{
    const QString fileName = QFileDialog::getSaveFileName(this, tr("Export Log"), QStringLiteral("openuasim-log.csv"),
                                                          tr("CSV files (*.csv)"));
    if (!fileName.isEmpty())
        exportLogTo(fileName, entries);
}

///
/// \brief Shows the project name and an unsaved-changes mark in the title.
///
void MainWindow::updateWindowTitle()
{
    setWindowTitle(QStringLiteral("%1[*] - %2").arg(_document->displayName(), QStringLiteral(APP_NAME)));
    setWindowModified(_document->isModified());
}

///
/// \brief Enables the node actions only for nodes of the project that no type declares.
///
void MainWindow::updateNodeActions()
{
    const NodeDefinition *node =
        _controller ? ProjectNodes::find(_controller->project().nodes, ui->objectsPage->currentNodeId()) : nullptr;
    const bool hasNode = node && !node->declared;
    ui->actionRenameNode->setEnabled(hasNode);
    ui->actionDuplicateNode->setEnabled(hasNode);
    ui->actionCopyNode->setEnabled(hasNode);
    ui->actionDeleteNode->setEnabled(hasNode);
    ui->actionPasteNode->setEnabled(_controller && ProjectNodes::find(_controller->project().nodes, _copiedNodeId));
}
