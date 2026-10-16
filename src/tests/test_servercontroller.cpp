// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QSignalSpy>
#include <QTest>

#include "project/defaultproject.h"
#include "server/servercontroller.h"
#include "testport.h"
#include "uatestclient.h"

Q_DECLARE_METATYPE(LogEntry)

namespace {

using Values = QHash<QString, QVariant>;

ProjectConfig project()
{
    ProjectConfig config = DefaultProject::create();
    config.endpoints.first().port = TestPort::freePort();
    return config;
}

bool logContains(const QList<LogEntry> &log, LogEntry::Category category, const QString &text)
{
    for (const LogEntry &entry : log) {
        if (entry.category == category && entry.message.contains(text))
            return true;
    }
    return false;
}

}

class TestServerController : public QObject
{
    Q_OBJECT

private slots:
    void setProjectBuildsNodes();
    void setProjectAssignsMissingIds();
    void setProjectLogsSkippedNodes();
    void startRunsSimulation();
    void stopFreezesSimulation();
    void restartAppliesChangedEndpoints();
    void startFailureIsReported();
    void clientWriteIsLoggedAndAnnounced();
    void sessionsAreForwarded();
    void snapshotIncludesLog();
    void logIsBounded();
};

void TestServerController::setProjectBuildsNodes()
{
    ServerController controller;
    QSignalSpy changed(&controller, &ServerController::projectChanged);
    controller.setProject(project());

    QCOMPARE(changed.count(), 1);
    QCOMPARE(controller.addressSpace().nodeCount(), 17);
    QCOMPARE(controller.snapshot().variables.size(), 4);
    QVERIFY(!controller.isRunning());
}

void TestServerController::setProjectAssignsMissingIds()
{
    ProjectConfig config = project();
    config.nodes.first().children.first().children.first().nodeId.clear();
    ServerController controller;
    controller.setProject(config);
    QCOMPARE(controller.project().nodes.first().children.first().children.first().nodeId,
             QStringLiteral("ns=1;s=FactoryLine_01.Motor_01.Speed"));
    QVERIFY(controller.addressSpace().hasVariable(QStringLiteral("ns=1;s=FactoryLine_01.Motor_01.Speed")));
}

void TestServerController::setProjectLogsSkippedNodes()
{
    ProjectConfig config = project();
    config.nodes.first().children.first().children.first().dataType = QStringLiteral("Guid");
    ServerController controller;
    controller.setProject(config);
    QVERIFY(logContains(controller.log(), LogEntry::Category::Server,
                        QStringLiteral("Node skipped: FactoryLine_01/Motor_01/Speed: unsupported data type 'Guid'")));
}

void TestServerController::startRunsSimulation()
{
    ServerController controller;
    controller.setProject(project());
    QSignalSpy running(&controller, &ServerController::runningChanged);
    QSignalSpy values(&controller, &ServerController::valuesChanged);

    QVERIFY(controller.start());
    QVERIFY(controller.isRunning());
    QCOMPARE(running.count(), 1);
    QTRY_VERIFY(values.count() > 0);
    QVERIFY(controller.status().running);
}

void TestServerController::stopFreezesSimulation()
{
    ServerController controller;
    controller.setProject(project());
    QVERIFY(controller.start());
    QSignalSpy values(&controller, &ServerController::valuesChanged);
    QTRY_VERIFY(values.count() > 0);

    controller.stop();
    QVERIFY(!controller.isRunning());
    values.clear();
    QTest::qWait(300);
    QCOMPARE(values.count(), 0);
}

void TestServerController::restartAppliesChangedEndpoints()
{
    ServerController controller;
    controller.setProject(project());
    QVERIFY(controller.start());

    ProjectConfig changed = controller.project();
    changed.endpoints.first().port = TestPort::freePort();
    controller.setProject(changed);
    QVERIFY(controller.restart());
    QCOMPARE(controller.server().endpointUrl(), changed.endpoints.first().url());

    UaTestClient client;
    QCOMPARE(client.connect(controller.server().endpointUrl()), UA_STATUSCODE_GOOD);
}

void TestServerController::startFailureIsReported()
{
    ProjectConfig config = project();
    config.endpoints.clear();
    ServerController controller;
    controller.setProject(config);
    QSignalSpy failed(&controller, &ServerController::startFailed);

    QVERIFY(!controller.start());
    QCOMPARE(failed.count(), 1);
    QVERIFY(logContains(controller.log(), LogEntry::Category::Server, QStringLiteral("No endpoint is enabled")));
}

void TestServerController::clientWriteIsLoggedAndAnnounced()
{
    ServerController controller;
    controller.setProject(project());
    QVERIFY(controller.start());
    QSignalSpy values(&controller, &ServerController::valuesChanged);

    UaTestClient client;
    QCOMPARE(client.connect(controller.server().endpointUrl()), UA_STATUSCODE_GOOD);
    QCOMPARE(client.writeValue(QStringLiteral("ns=1;s=Tank_01.InletValve"), true), UA_STATUSCODE_GOOD);

    QTRY_VERIFY(logContains(controller.log(), LogEntry::Category::DataChanges,
                            QStringLiteral("to ns=1;s=Tank_01.InletValve")));
    bool announced = false;
    for (const QList<QVariant> &arguments : std::as_const(values))
        announced = announced || arguments.first().value<Values>().contains(QStringLiteral("ns=1;s=Tank_01.InletValve"));
    QVERIFY(announced);
}

void TestServerController::sessionsAreForwarded()
{
    ServerController controller;
    controller.setProject(project());
    QVERIFY(controller.start());
    QSignalSpy sessions(&controller, &ServerController::sessionsChanged);

    UaTestClient client;
    QCOMPARE(client.connect(controller.server().endpointUrl()), UA_STATUSCODE_GOOD);
    QTRY_VERIFY(sessions.count() > 0);
    QCOMPARE(controller.sessions().size(), 1);
    QCOMPARE(controller.status().clientsConnected, 1);
}

void TestServerController::snapshotIncludesLog()
{
    ServerController controller;
    controller.setProject(project());
    QVERIFY(controller.start());
    const ServerSnapshot snapshot = controller.snapshot();
    QCOMPARE(snapshot.log.size(), controller.log().size());
    QVERIFY(logContains(snapshot.log, LogEntry::Category::Server, QStringLiteral("Server started")));
}

void TestServerController::logIsBounded()
{
    ProjectConfig config = project();
    config.endpoints.clear();
    ServerController controller;
    controller.setProject(config);
    for (int i = 0; i < ServerController::kLogLimit + 10; ++i)
        controller.start();
    QCOMPARE(controller.log().size(), ServerController::kLogLimit);
}

QTEST_GUILESS_MAIN(TestServerController)

#include "test_servercontroller.moc"
