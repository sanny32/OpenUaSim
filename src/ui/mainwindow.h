// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file mainwindow.h
/// \brief Declares the main application window.
///

#pragma once

#include <QMainWindow>

#include "data/certificateinfo.h"

namespace Ui {
class MainWindow;
}

class AppTheme;
class LogModel;
class MainStatusBarWidget;
class QAction;
class QActionGroup;
class ProjectDocument;
class QLabel;
struct EndpointInfo;
class QTimer;
class ServerController;
struct LogEntry;
struct ServerSnapshot;
struct ServerStatus;
struct VariableInfo;

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
    /// \brief Imports a NodeSet2 model, reporting a file that cannot be loaded.
    /// \param fileName NodeSet2 file.
    /// \return True when the model was loaded.
    ///
    bool importModelFrom(const QString &fileName);

    ///
    /// \brief Exports the project nodes as NodeSet2, reporting a file that cannot be written.
    /// \param fileName Target path.
    /// \return True when the file was written.
    ///
    bool exportModelTo(const QString &fileName);

    ///
    /// \brief Exports the nodes of one namespace as NodeSet2, reporting a file that cannot be written.
    /// \param fileName Target path.
    /// \param namespaceUri URI of the namespace to export.
    /// \return True when the file was written.
    ///
    bool exportNamespaceTo(const QString &fileName, const QString &namespaceUri);

    ///
    /// \brief Writes log entries to a CSV file, reporting a file that cannot be written.
    /// \param fileName Target path.
    /// \param entries Entries to write.
    /// \return True when the file was written.
    ///
    bool exportLogTo(const QString &fileName, const QList<LogEntry> &entries);

    ///
    /// \brief Adds a client or issuer certificate to the PKI store, reporting a refused file.
    /// \param fileName PEM or DER certificate.
    /// \param category Trusted, Rejected or Issuer folder.
    /// \return True when the certificate was stored.
    ///
    bool importCertificateFrom(const QString &fileName, CertificateInfo::Category category);

    ///
    /// \brief Installs the server certificate and key, reporting refused files.
    /// \param certificateFile PEM or DER certificate.
    /// \param keyFile PEM or DER private key.
    /// \return True when both were stored.
    ///
    bool installServerCertificateFrom(const QString &certificateFile, const QString &keyFile);

    ///
    /// \brief Returns the log shared by the event log panel and the log page.
    /// \return Server log model.
    ///
    LogModel *logModel() const;

protected:
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void setupIcons();
    void setupActions();
    void setupViewMenu();
    void selectPage(QAction *action);
    void showAbout();
    void setupThemeMenu();
    void syncThemeActions();
    void selectTheme(QAction *action);
    void handleTabChange(int index);
    void updateEventLogVisibility();
    void updateStatusBarNote();
    void applyStatus(const ServerStatus &status);
    void refreshStatus();
    void reloadSnapshot();
    void appendLog(const LogEntry &entry);
    void refreshSessions();
    void showStartFailure(const QString &reason);
    void showStartWarning(const QString &reason);
    void showError(const QString &title, const QString &text);
    void showAddNodeDialog();
    void addNodeFromDialog();
    void confirmDeleteNode();
    void deletePendingNode();
    void showRenameDialog();
    void renamePendingNode(const QString &browseName);
    void showServerSettingsDialog();
    void applyServerSettingsFromDialog();
    void applyVariableChange(const VariableInfo &variable);
    void updateNodeActions();
    void applyEndpoints(const QList<EndpointInfo> &endpoints);
    void reloadEndpoints();
    void updateRestartNote(bool required);
    void newProject();
    void openProject();
    bool saveProject();
    bool saveProjectAs();
    void updateWindowTitle();
    void importModel();
    void exportModel();
    void exportNamespace(const QString &namespaceUri);
    void exportLog(const QList<LogEntry> &entries);
    void confirmRemoveModel(const QString &fileName);
    void removePendingModel();
    void importCertificate(CertificateInfo::Category category);
    void exportCertificate(const QString &filePath);
    void moveCertificate(const QString &filePath, CertificateInfo::Category category);
    void confirmDeleteCertificate(const QString &filePath);
    void deletePendingCertificate();
    void confirmGenerateServerCertificate();
    void generateAfterConfirmation();
    void generateServerCertificate();
    void reloadCertificates();

    Ui::MainWindow *ui;
    MainStatusBarWidget *_statusWidget;
    LogModel *_logModel;
    AppTheme *_theme;
    ServerController *_controller = nullptr;
    ProjectDocument *_document = nullptr;
    QTimer *_statusTimer;
    QLabel *_restartNote;
    QString _pendingParentId;
    QString _pendingDeleteId;
    QString _pendingRenameId;
    QString _pendingModelFile;
    QString _pendingCertificate;
    QActionGroup *_pageActions = nullptr;
    bool _contentSplitApplied = false;
};
