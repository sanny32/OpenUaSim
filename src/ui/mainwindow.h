// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file mainwindow.h
/// \brief Declares the main application window.
///

#pragma once

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class AppTheme;
class LogModel;
class MainStatusBarWidget;
class QAction;
class QActionGroup;
class QTimer;
class ServerController;
struct LogEntry;
struct ServerSnapshot;
struct ServerStatus;

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
    /// \brief Returns the log shared by the event log panel and the log page.
    /// \return Server log model.
    ///
    LogModel *logModel() const;

protected:
    void showEvent(QShowEvent *event) override;

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

    Ui::MainWindow *ui;
    MainStatusBarWidget *_statusWidget;
    LogModel *_logModel;
    AppTheme *_theme;
    ServerController *_controller = nullptr;
    QTimer *_statusTimer;
    QActionGroup *_pageActions = nullptr;
    bool _contentSplitApplied = false;
};
