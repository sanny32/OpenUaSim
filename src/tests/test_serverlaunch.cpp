// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QTest>

#include "server/servercontroller.h"
#include "server/serverlaunch.h"
#include "settingsstore.h"
#include "testport.h"
#include "testproject.h"

namespace {

ProjectConfig project()
{
    ProjectConfig config = TestProject::create();
    config.endpoints.first().port = TestPort::freePort();
    return config;
}

}

class TestServerLaunch : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void staysStoppedWhenNeverRemembered();
    void remembersState();
    void restoreStartsRememberedRunningServer();
    void restoreLeavesStoppedServer();
    void failedRestoreKeepsRememberedState();
    void restoreEnabledByDefault();
    void disabledRestoreLeavesRunningStateStopped();
};

void TestServerLaunch::init()
{
    SettingsStore().clear();
}

void TestServerLaunch::staysStoppedWhenNeverRemembered()
{
    QVERIFY(!ServerLaunch::shouldStart());
    ServerController controller;
    controller.setProject(project());
    QVERIFY(!ServerLaunch::restore(controller));
    QVERIFY(!controller.isRunning());
}

void TestServerLaunch::remembersState()
{
    ServerLaunch::remember(false);
    QVERIFY(!ServerLaunch::shouldStart());
    ServerLaunch::remember(true);
    QVERIFY(ServerLaunch::shouldStart());
}

void TestServerLaunch::restoreStartsRememberedRunningServer()
{
    ServerLaunch::remember(true);
    ServerController controller;
    controller.setProject(project());
    QVERIFY(ServerLaunch::restore(controller));
    QVERIFY(controller.isRunning());
}

void TestServerLaunch::restoreLeavesStoppedServer()
{
    ServerLaunch::remember(false);
    ServerController controller;
    controller.setProject(project());
    QVERIFY(!ServerLaunch::restore(controller));
    QVERIFY(!controller.isRunning());
}

void TestServerLaunch::failedRestoreKeepsRememberedState()
{
    ServerLaunch::remember(true);
    ProjectConfig config = project();
    config.endpoints.clear();
    ServerController controller;
    controller.setProject(config);
    QVERIFY(!ServerLaunch::restore(controller));
    QVERIFY(!controller.isRunning());
    QVERIFY(ServerLaunch::shouldStart());
}

void TestServerLaunch::restoreEnabledByDefault()
{
    QVERIFY(ServerLaunch::isRestoreEnabled());
    ServerLaunch::setRestoreEnabled(false);
    QVERIFY(!ServerLaunch::isRestoreEnabled());
    ServerLaunch::setRestoreEnabled(true);
    QVERIFY(ServerLaunch::isRestoreEnabled());
}

void TestServerLaunch::disabledRestoreLeavesRunningStateStopped()
{
    ServerLaunch::remember(true);
    ServerLaunch::setRestoreEnabled(false);
    ServerController controller;
    controller.setProject(project());
    QVERIFY(!ServerLaunch::restore(controller));
    QVERIFY(!controller.isRunning());
    QVERIFY(ServerLaunch::shouldStart());
}

QTEST_GUILESS_MAIN(TestServerLaunch)

#include "test_serverlaunch.moc"
