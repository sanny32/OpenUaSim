// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file mainwindow.h
/// \brief Declares the main application window.
///

#pragma once

#include <QHash>
#include <QMainWindow>
#include <QStringList>

#include "data/typeinfo.h"
#include "data/variableinfo.h"

namespace Ui {
class MainWindow;
}

class AppTheme;
class CertificateActions;
class LogModel;
class MainStatusBarWidget;
class ModelActions;
class QAction;
class QActionGroup;
class ProjectDocument;
struct EndpointInfo;
struct NodeDefinition;
class QTimer;
class ServerController;
struct LogEntry;
struct ServerSnapshot;
struct ServerStatus;
struct SessionInfo;

///
/// \brief Main window: menu bar, server toolbar, the eight pages, the event log and the status bar.
///
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the window.
    /// \param theme Theme manager driven by the theme menu; null hides the theme button.
    /// \param parent Parent widget.
    ///
    explicit MainWindow(AppTheme *theme = nullptr, QWidget *parent = nullptr);

    ///
    /// \brief Destroys the window.
    ///
    ~MainWindow() override;

    ///
    /// \brief Shows a server state on every page.
    /// \param snapshot Server state to show.
    ///
    void setSnapshot(const ServerSnapshot &snapshot);

    ///
    /// \brief Binds the window to the server it controls and shows its state.
    /// \param controller Controller serving the project; must outlive the window.
    ///
    void setController(ServerController *controller);

    ///
    /// \brief Binds the File menu to the project document and shows its name in the title.
    /// \param document Document of the served project; must outlive the window.
    ///
    void setDocument(ProjectDocument *document);

    ///
    /// \brief Serves a project file, reporting a file that cannot be read.
    /// \param fileName File to open.
    /// \return True when the file was opened.
    ///
    bool openProjectFile(const QString &fileName);

    ///
    /// \brief Saves the project to a file, reporting a file that cannot be written.
    /// \param fileName Target path.
    /// \return True when the file was written.
    ///
    bool saveProjectTo(const QString &fileName);

    ///
    /// \brief Asks whether unsaved changes may be dropped, offering to save them first.
    /// \return True when nothing is unsaved, the changes were saved or the user discards them.
    ///
    bool confirmDiscardChanges();

    ///
    /// \brief Writes log entries to a CSV file, reporting a file that cannot be written.
    /// \param fileName Target path.
    /// \param entries Entries to write.
    /// \return True when the file was written.
    ///
    bool exportLogTo(const QString &fileName, const QList<LogEntry> &entries);

    ///
    /// \brief Returns the log shared by the event log panel and the log page.
    /// \return Server log model.
    ///
    LogModel *logModel() const;

    ///
    /// \brief Returns the actions of the Certificates page.
    /// \return Actions created by setController(), or null before.
    ///
    CertificateActions *certificateActions() const;

    ///
    /// \brief Returns the actions on NodeSet2 models.
    /// \return Actions created by setController(), or null before.
    ///
    ModelActions *modelActions() const;

protected:
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    void restoreSettings();
    void saveSettings();
    void setupIcons();
    void setupActions();
    void setupViewMenu();
    void selectPage(QAction *action);
    void showAbout();
    void setupThemeMenu();
    void setupLanguageMenu();
    void selectLanguage(QAction *action);
    void retranslate();
    void syncThemeActions();
    void selectTheme(QAction *action);
    void handleTabChange(int index);
    void updateEventLogVisibility();
    void hideEventLog();
    void updateStatusBarNote();
    void applyStatus(const ServerStatus &status);
    void refreshStatus();
    void reloadSnapshot();
    void appendLog(const LogEntry &entry);
    void refreshSessions();
    void startServer();
    void stopServer();
    void showStartFailure(const QString &reason);
    void showStartWarning(const QString &reason);
    void showError(const QString &title, const QString &text);
    void showAddNodeDialog();
    void addNodeFromDialog();
    void confirmDeleteNode();
    void deletePendingNode();
    const NodeDefinition *targetContainer() const;
    void duplicateNode();
    void copyNode();
    void pasteNode();
    void copyNodeTo(const QString &nodeId, const QString &parentId);
    void showRenameDialog();
    void renamePendingNode(const QString &browseName);
    void showServerSettingsDialog();
    void applyServerSettingsFromDialog();
    void showAlarmDialog(const VariableInfo &variable);
    void applyAlarmFromDialog();
    void showUsersDialog();
    void applyUsersFromDialog();
    void applyVariableChange(const VariableInfo &variable);
    void updateNodeActions();
    void applyEndpoints(const QList<EndpointInfo> &endpoints);
    void reloadEndpoints();
    void offerRestart();
    void offerRestartIfRequired();
    void newProject();
    void openProject();
    bool saveProject();
    bool saveProjectAs();
    void updateWindowTitle();
    void exportLog(const QList<LogEntry> &entries);
    void showSubscriptions(const SessionInfo &session);

    Ui::MainWindow *ui;
    MainStatusBarWidget *_statusWidget;
    LogModel *_logModel;
    AppTheme *_theme;
    ServerController *_controller = nullptr;
    ProjectDocument *_document = nullptr;
    QTimer *_statusTimer;
    QStringList _pendingDeleteIds;
    VariableInfo _pendingAlarmVariable;
    QString _copiedNodeId;
    QHash<QString, TypeInfo> _types;
    QStringList _namespaces;
    QString _pendingRenameId;
    QActionGroup *_pageActions = nullptr;
    CertificateActions *_certificateActions = nullptr;
    ModelActions *_modelActions = nullptr;
    bool _contentSplitApplied = false;
};
