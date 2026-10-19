// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file mainwindow.cpp
/// \brief Implements the main application window.
///

#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QTimer>

#include "addnodedialog.h"
#include "appicons.h"
#include "apptheme.h"
#include "data/serversnapshot.h"
#include "logmodel.h"
#include "mainstatusbarwidget.h"
#include "mainwindow.h"
#include "project/projectdocument.h"
#include "project/projectfile.h"
#include "project/projectnodes.h"
#include "server/servercontroller.h"
#include "ui_mainwindow.h"
#include "viewsetup.h"

namespace {

constexpr double kEventLogShare = 0.28;
constexpr int kPageShortcutCount = 9;
constexpr int kStatusRefreshMs = 1000;

///
/// \brief Returns the file name filter of NodeSet2 files for file dialogs.
/// \return Filter such as "NodeSet2 XML (*.xml)".
///
QString nodeSetFilter()
{
    return QCoreApplication::translate("MainWindow", "NodeSet2 XML (*.xml)");
}

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
    , _restartNote(new QLabel(tr("Restart the server to apply the changes"), this))
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
    connect(controller, &ServerController::startWarning, this, &MainWindow::showStartWarning);
    connect(controller, &ServerController::variableChanged, ui->objectsPage, &ObjectsPage::updateVariable);
    connect(ui->objectsPage, &ObjectsPage::variableChangeRequested, this, &MainWindow::applyVariableChange);
    connect(ui->objectsPage, &ObjectsPage::currentNodeChanged, this, &MainWindow::updateNodeActions);
    connect(ui->actionAddNode, &QAction::triggered, this, &MainWindow::showAddNodeDialog);
    connect(ui->actionDeleteNode, &QAction::triggered, this, &MainWindow::confirmDeleteNode);
    connect(ui->certificatesPage, &CertificatesPage::generateRequested,
            this, &MainWindow::confirmGenerateServerCertificate);
    connect(ui->certificatesPage, &CertificatesPage::importRequested, this, &MainWindow::importCertificate);
    connect(ui->certificatesPage, &CertificatesPage::exportRequested, this, &MainWindow::exportCertificate);
    connect(ui->certificatesPage, &CertificatesPage::moveRequested, this, &MainWindow::moveCertificate);
    connect(ui->certificatesPage, &CertificatesPage::deleteRequested, this, &MainWindow::confirmDeleteCertificate);
    connect(controller, &ServerController::certificatesChanged, this, &MainWindow::reloadCertificates);
    connect(ui->actionImportModel, &QAction::triggered, this, &MainWindow::importModel);
    connect(ui->actionExportModel, &QAction::triggered, this, &MainWindow::exportModel);
    connect(ui->endpointsPage, &EndpointsPage::endpointsChangeRequested, this, &MainWindow::applyEndpoints);
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
    const QMessageBox::StandardButton answer = QMessageBox::warning(
        this, tr("Unsaved Changes"), tr("Save the changes to '%1'?").arg(_document->displayName()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (answer == QMessageBox::Save)
        return saveProject();
    return answer == QMessageBox::Discard;
}

///
/// \brief Imports a NodeSet2 model, reporting a file that cannot be loaded.
/// \param fileName NodeSet2 file.
/// \return True when the model was loaded.
///
bool MainWindow::importModelFrom(const QString &fileName)
{
    QString error;
    if (_controller && _controller->importNodeSet(fileName, &error))
        return true;
    showError(tr("Model not imported"), error);
    return false;
}

///
/// \brief Exports the project nodes as NodeSet2, reporting a file that cannot be written.
/// \param fileName Target path.
/// \return True when the file was written.
///
bool MainWindow::exportModelTo(const QString &fileName)
{
    QString error;
    if (_controller && _controller->exportNodeSet(fileName, &error))
        return true;
    showError(tr("Model not exported"), error);
    return false;
}

///
/// \brief Adds a client or issuer certificate to the PKI store, reporting a refused file.
/// \param fileName PEM or DER certificate.
/// \param category Trusted, Rejected or Issuer folder.
/// \return True when the certificate was stored.
///
bool MainWindow::importCertificateFrom(const QString &fileName, CertificateInfo::Category category)
{
    QString error;
    if (_controller && _controller->importCertificate(fileName, category, &error))
        return true;
    showError(tr("Certificate not imported"), error);
    return false;
}

///
/// \brief Installs the server certificate and key, reporting refused files.
/// \param certificateFile PEM or DER certificate.
/// \param keyFile PEM or DER private key.
/// \return True when both were stored.
///
bool MainWindow::installServerCertificateFrom(const QString &certificateFile, const QString &keyFile)
{
    QString error;
    if (_controller && _controller->installServerCertificate(certificateFile, keyFile, &error))
        return true;
    showError(tr("Server certificate not installed"), error);
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

    ui->objectsPage->setTreeActions({ ui->actionAddNode, ui->actionDeleteNode });
    ui->actionDeleteNode->setEnabled(false);
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
    auto *box = new QMessageBox(QMessageBox::Warning, title, text, QMessageBox::Ok, this);
    box->setAttribute(Qt::WA_DeleteOnClose);
    box->open();
}

///
/// \brief Opens the Add Node dialog for the selected folder or object, or for the top level.
///
void MainWindow::showAddNodeDialog()
{
    if (!_controller)
        return;
    const QList<NodeDefinition> &nodes = _controller->project().nodes;
    const NodeDefinition *selected = ProjectNodes::find(nodes, ui->objectsPage->currentNodeId());
    if (selected && selected->kind == NodeDefinition::Kind::Variable)
        selected = ProjectNodes::find(nodes, ui->objectsPage->currentContainerId());
    _pendingParentId = selected ? selected->nodeId : QString();

    auto *dialog = new AddNodeDialog(selected ? selected->browseName : tr("Objects"),
                                     ProjectNodes::childNames(nodes, _pendingParentId),
                                     ProjectNodes::nodeIds(nodes), this);
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
    const QString nodeId = _controller->addNode(_pendingParentId, dialog->node(), &error);
    if (nodeId.isEmpty()) {
        showError(tr("Node not added"), error);
        return;
    }
    ui->objectsPage->selectNode(nodeId);
}

///
/// \brief Asks before deleting the selected project node with its children.
///
void MainWindow::confirmDeleteNode()
{
    if (!_controller)
        return;
    const NodeDefinition *node = ProjectNodes::find(_controller->project().nodes, ui->objectsPage->currentNodeId());
    if (!node)
        return;
    _pendingDeleteId = node->nodeId;
    auto *box = new QMessageBox(QMessageBox::Question, tr("Delete Node"),
                                tr("Delete '%1' and everything below it?").arg(node->browseName),
                                QMessageBox::Yes | QMessageBox::Cancel, this);
    box->setAttribute(Qt::WA_DeleteOnClose);
    connect(box, &QDialog::finished, this, &MainWindow::deletePendingNode);
    box->open();
}

///
/// \brief Deletes the node the Delete Node question was answered Yes for.
///
void MainWindow::deletePendingNode()
{
    const auto *box = qobject_cast<const QMessageBox *>(sender());
    if (_controller && box && box->standardButton(box->clickedButton()) == QMessageBox::Yes)
        _controller->removeNode(_pendingDeleteId);
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
/// \brief Picks a NodeSet2 file and imports it.
///
void MainWindow::importModel()
{
    if (!_controller)
        return;
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Import Model"), QString(),
                                                          nodeSetFilter());
    if (!fileName.isEmpty())
        importModelFrom(fileName);
}

///
/// \brief Asks for a file name and exports the project nodes there.
///
void MainWindow::exportModel()
{
    if (!_controller)
        return;
    const QString fileName = QFileDialog::getSaveFileName(this, tr("Export Model"), QString(),
                                                          nodeSetFilter());
    if (!fileName.isEmpty())
        exportModelTo(fileName);
}

///
/// \brief Picks a certificate, and for the server folder also its private key, and stores it.
/// \param category Folder the certificate goes to.
///
void MainWindow::importCertificate(CertificateInfo::Category category)
{
    const QString filter = tr("Certificates (*.der *.pem *.crt *.cer);;All files (*)");
    const QString certificate = QFileDialog::getOpenFileName(this, tr("Import Certificate"), QString(), filter);
    if (certificate.isEmpty())
        return;
    if (category != CertificateInfo::Category::Application) {
        importCertificateFrom(certificate, category);
        return;
    }
    const QString key = QFileDialog::getOpenFileName(this, tr("Private Key of the Server Certificate"), QString(),
                                                     tr("Private keys (*.pem *.key *.der);;All files (*)"));
    if (!key.isEmpty())
        installServerCertificateFrom(certificate, key);
}

///
/// \brief Saves a copy of a stored certificate.
/// \param filePath Stored certificate file.
///
void MainWindow::exportCertificate(const QString &filePath)
{
    const QString target = QFileDialog::getSaveFileName(this, tr("Export Certificate"), QFileInfo(filePath).fileName(),
                                                        tr("DER certificates (*.der)"));
    if (target.isEmpty())
        return;
    QFile::remove(target);
    if (!QFile::copy(filePath, target))
        showError(tr("Certificate not exported"), tr("Cannot write '%1'").arg(target));
}

///
/// \brief Trusts or rejects a stored certificate.
/// \param filePath Stored certificate file.
/// \param category Target folder.
///
void MainWindow::moveCertificate(const QString &filePath, CertificateInfo::Category category)
{
    QString error;
    if (_controller && !_controller->moveCertificate(filePath, category, &error))
        showError(tr("Certificate not moved"), error);
}

///
/// \brief Asks before deleting a stored certificate.
/// \param filePath Stored certificate file.
///
void MainWindow::confirmDeleteCertificate(const QString &filePath)
{
    _pendingCertificate = filePath;
    auto *box = new QMessageBox(QMessageBox::Question, tr("Delete Certificate"),
                                tr("Delete '%1'?").arg(QFileInfo(filePath).fileName()),
                                QMessageBox::Yes | QMessageBox::Cancel, this);
    box->setAttribute(Qt::WA_DeleteOnClose);
    connect(box, &QDialog::finished, this, &MainWindow::deletePendingCertificate);
    box->open();
}

///
/// \brief Deletes the certificate the Delete Certificate question was answered Yes for.
///
void MainWindow::deletePendingCertificate()
{
    const auto *box = qobject_cast<const QMessageBox *>(sender());
    if (!_controller || !box || box->standardButton(box->clickedButton()) != QMessageBox::Yes)
        return;
    QString error;
    if (!_controller->removeCertificate(_pendingCertificate, &error))
        showError(tr("Certificate not deleted"), error);
}

///
/// \brief Creates a server certificate, first asking before it replaces an installed one.
///
void MainWindow::confirmGenerateServerCertificate()
{
    if (!_controller)
        return;
    if (!_controller->certificateStore().hasServerCertificate()) {
        generateServerCertificate();
        return;
    }
    auto *box = new QMessageBox(QMessageBox::Question, tr("Generate Server Certificate"),
                                tr("Replace the installed server certificate? Clients that trust it "
                                   "will have to trust the new one."),
                                QMessageBox::Yes | QMessageBox::Cancel, this);
    box->setAttribute(Qt::WA_DeleteOnClose);
    connect(box, &QDialog::finished, this, &MainWindow::generateAfterConfirmation);
    box->open();
}

///
/// \brief Creates the server certificate when the replacement question was answered Yes.
///
void MainWindow::generateAfterConfirmation()
{
    const auto *box = qobject_cast<const QMessageBox *>(sender());
    if (box && box->standardButton(box->clickedButton()) == QMessageBox::Yes)
        generateServerCertificate();
}

///
/// \brief Creates and installs a self-signed server certificate, reporting a failure.
///
void MainWindow::generateServerCertificate()
{
    QString error;
    if (_controller && !_controller->generateServerCertificate(&error))
        showError(tr("Server certificate not created"), error);
}

///
/// \brief Shows the current contents of the PKI store.
///
void MainWindow::reloadCertificates()
{
    if (_controller)
        ui->certificatesPage->setCertificates(_controller->certificates());
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
/// \brief Enables Delete Node only for nodes of the project.
///
void MainWindow::updateNodeActions()
{
    ui->actionDeleteNode->setEnabled(
        _controller && ProjectNodes::find(_controller->project().nodes, ui->objectsPage->currentNodeId()));
}
