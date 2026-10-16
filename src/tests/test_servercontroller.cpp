// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QSignalSpy>
#include <QTest>

#include "project/defaultproject.h"
#include "project/projectnodes.h"
#include "server/servercontroller.h"
#include "server/snapshotbuilder.h"
#include "testport.h"
#include "uatestclient.h"

Q_DECLARE_METATYPE(LogEntry)
Q_DECLARE_METATYPE(VariableInfo)

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
    void addNodeRebuildsAddressSpace();
    void addNodeReportsRefusal();
    void removeNodeRebuildsAddressSpace();
    void updateVariableAppliesLiveSettings();
    void updateVariableRenameRebuilds();
    void updateVariableReportsRefusal();
    void setEndpointsWhileStopped();
    void setEndpointsWhileRunningRequiresRestart();
    void setEndpointsRejectsConflicts();
    void setEndpointsAcceptsAllDisabled();
};

void TestServerController::setEndpointsWhileStopped()
{
    ServerController controller;
    controller.setProject(project());
    QSignalSpy changed(&controller, &ServerController::endpointsChanged);
    QList<EndpointInfo> endpoints = controller.project().endpoints;
    endpoints.first().name = QStringLiteral("Renamed");
    QVERIFY(controller.setEndpoints(endpoints));
    QCOMPARE(changed.count(), 1);
    QCOMPARE(controller.project().endpoints.first().name, QStringLiteral("Renamed"));
    QVERIFY(!controller.isRestartRequired());
}

void TestServerController::setEndpointsWhileRunningRequiresRestart()
{
    ServerController controller;
    controller.setProject(project());
    QVERIFY(controller.start());
    QSignalSpy required(&controller, &ServerController::restartRequiredChanged);

    QList<EndpointInfo> endpoints = controller.project().endpoints;
    endpoints.first().port = TestPort::freePort();
    QVERIFY(controller.setEndpoints(endpoints));
    QVERIFY(controller.isRestartRequired());
    QCOMPARE(required.count(), 1);

    QVERIFY(controller.restart());
    QVERIFY(!controller.isRestartRequired());
    QCOMPARE(controller.server().endpointUrl(), endpoints.first().url());
    QCOMPARE(required.count(), 2);
}

void TestServerController::setEndpointsRejectsConflicts()
{
    ServerController controller;
    controller.setProject(project());
    QList<EndpointInfo> endpoints = controller.project().endpoints;
    endpoints.append(endpoints.first());
    endpoints.last().port = endpoints.first().port + 1;
    QString error;
    QVERIFY(!controller.setEndpoints(endpoints, &error));
    QVERIFY(error.contains(QStringLiteral("must share one port")));
    QCOMPARE(controller.project().endpoints.size(), 1);
}

void TestServerController::setEndpointsAcceptsAllDisabled()
{
    ServerController controller;
    controller.setProject(project());
    QList<EndpointInfo> endpoints = controller.project().endpoints;
    endpoints.first().enabled = false;
    QVERIFY(controller.setEndpoints(endpoints));
    QVERIFY(controller.setEndpoints({}));
    QVERIFY(!controller.start());
}

void TestServerController::addNodeRebuildsAddressSpace()
{
    ServerController controller;
    controller.setProject(project());
    QSignalSpy changed(&controller, &ServerController::projectChanged);

    NodeDefinition outlet;
    outlet.kind = NodeDefinition::Kind::Variable;
    outlet.browseName = QStringLiteral("Outlet");
    const QString id = controller.addNode(QStringLiteral("ns=1;s=Tank_01"), outlet);
    QCOMPARE(id, QStringLiteral("ns=1;s=FactoryLine_01.Tank_01.Outlet"));
    QCOMPARE(changed.count(), 1);
    QVERIFY(controller.addressSpace().hasVariable(id));
    QCOMPARE(controller.addressSpace().nodeCount(), 18);
}

void TestServerController::addNodeReportsRefusal()
{
    ServerController controller;
    controller.setProject(project());
    QSignalSpy changed(&controller, &ServerController::projectChanged);
    NodeDefinition unnamed;
    QString error;
    QVERIFY(controller.addNode(QString(), unnamed, &error).isEmpty());
    QCOMPARE(error, QStringLiteral("The browse name is empty"));
    QCOMPARE(changed.count(), 0);
}

void TestServerController::removeNodeRebuildsAddressSpace()
{
    ServerController controller;
    controller.setProject(project());
    QVERIFY(controller.removeNode(QStringLiteral("ns=1;s=Motor_01")));
    QVERIFY(!controller.addressSpace().hasVariable(QStringLiteral("ns=1;s=Motor_01.Speed")));
    QCOMPARE(controller.addressSpace().nodeCount(), 13);
    QVERIFY(!controller.removeNode(QStringLiteral("ns=1;s=Motor_01")));
}

void TestServerController::updateVariableAppliesLiveSettings()
{
    ServerController controller;
    controller.setProject(project());
    QVERIFY(controller.start());
    QSignalSpy structure(&controller, &ServerController::projectChanged);
    QSignalSpy variable(&controller, &ServerController::variableChanged);

    VariableInfo valve = SnapshotBuilder::variableInfo(
        *ProjectNodes::find(controller.project().nodes, QStringLiteral("ns=1;s=Tank_01.InletValve")),
        controller.addressSpace());
    valve.access = QStringLiteral("Read");
    valve.simulation.function = SimulationFunction::Square;
    valve.simulation.intervalMs = 50;
    QVERIFY(controller.updateVariable(valve));

    QCOMPARE(structure.count(), 0);
    QCOMPARE(variable.count(), 1);
    QCOMPARE(variable.first().first().value<VariableInfo>().access, QStringLiteral("Read"));
    QCOMPARE(ProjectNodes::find(controller.project().nodes, valve.nodeId)->simulation.function,
             SimulationFunction::Square);

    UaTestClient client;
    QCOMPARE(client.connect(controller.server().endpointUrl()), UA_STATUSCODE_GOOD);
    QVERIFY(client.writeValue(valve.nodeId, true) != UA_STATUSCODE_GOOD);
}

void TestServerController::updateVariableRenameRebuilds()
{
    ServerController controller;
    controller.setProject(project());
    QSignalSpy structure(&controller, &ServerController::projectChanged);

    VariableInfo level;
    level.nodeId = QStringLiteral("ns=1;s=Tank_01.Level");
    level.displayName = QStringLiteral("FillLevel");
    level.dataType = QStringLiteral("Float");
    level.access = QStringLiteral("Read/Write");
    QVERIFY(controller.updateVariable(level));
    QCOMPARE(structure.count(), 1);
    QCOMPARE(controller.addressSpace().value(level.nodeId).typeId(), int(QMetaType::Float));
}

void TestServerController::updateVariableReportsRefusal()
{
    ServerController controller;
    controller.setProject(project());
    VariableInfo unknown;
    unknown.nodeId = QStringLiteral("ns=1;s=None");
    QString error;
    QVERIFY(!controller.updateVariable(unknown, &error));
    QCOMPARE(error, QStringLiteral("'ns=1;s=None' is not a variable"));
}

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
