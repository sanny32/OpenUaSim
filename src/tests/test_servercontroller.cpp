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
    void removeNodesRebuildsOnce();
    void copyNodeDuplicatesSubtree();
    void updateVariableAppliesLiveSettings();
    void updateVariableRenameRebuilds();
    void updateVariableReportsRefusal();
    void updateVariableAppliesInitialValueAndQuality();
    void updateVariableRejectsInvalidInitialValue();
    void renameNodeRebuilds();
    void setServerSettingsRequiresRestartWhileRunning();
    void setServerSettingsRejectsInvalidValues_data();
    void setServerSettingsRejectsInvalidValues();
    void setUsersValidatesAndRequiresRestart();
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
    endpoints.first().hostname = QStringLiteral("renamed-host");
    QVERIFY(controller.setEndpoints(endpoints));
    QCOMPARE(changed.count(), 1);
    QCOMPARE(controller.project().endpoints.first().hostname, QStringLiteral("renamed-host"));
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

void TestServerController::removeNodesRebuildsOnce()
{
    ServerController controller;
    controller.setProject(project());
    QSignalSpy structure(&controller, &ServerController::projectChanged);
    QVERIFY(controller.removeNodes({ QStringLiteral("ns=1;s=Motor_01"), QStringLiteral("ns=1;s=Motor_01.Speed"),
                                     QStringLiteral("ns=1;s=Pump_01") }));
    QCOMPARE(structure.count(), 1);
    QCOMPARE(controller.addressSpace().nodeCount(), 9);
    QVERIFY(!controller.removeNodes({ QStringLiteral("ns=1;s=Motor_01") }));
    QCOMPARE(structure.count(), 1);
}

void TestServerController::copyNodeDuplicatesSubtree()
{
    ServerController controller;
    controller.setProject(project());
    const QString line = QStringLiteral("ns=1;s=FactoryLine_01");
    const QString motor = QStringLiteral("ns=1;s=Motor_01");

    const QString copyId = controller.copyNode(motor, line);
    QVERIFY(!copyId.isEmpty());
    QVERIFY(copyId != motor);
    const NodeDefinition *copy = ProjectNodes::find(controller.project().nodes, copyId);
    QCOMPARE(copy->browseName, QStringLiteral("Motor_01_copy"));
    QCOMPARE(ProjectNodes::parentId(controller.project().nodes, copyId), line);
    QCOMPARE(copy->children.size(), ProjectNodes::find(controller.project().nodes, motor)->children.size());
    QVERIFY(controller.addressSpace().hasVariable(copy->children.first().nodeId));
    QCOMPARE(controller.addressSpace().nodeCount(), 17 + 1 + int(copy->children.size()));

    const QString intoPump = controller.copyNode(QStringLiteral("ns=1;s=Motor_01.Speed"), QStringLiteral("ns=1;s=Pump_01"));
    QCOMPARE(ProjectNodes::find(controller.project().nodes, intoPump)->browseName, QStringLiteral("Speed"));

    QString error;
    QVERIFY(controller.copyNode(QStringLiteral("ns=1;s=None"), line, &error).isEmpty());
    QCOMPARE(error, QStringLiteral("There is no node 'ns=1;s=None'"));
    QVERIFY(controller.copyNode(motor, QStringLiteral("ns=1;s=Motor_01.Speed"), &error).isEmpty());
    QVERIFY(error.contains(QStringLiteral("cannot hold child nodes")));
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
    valve.icon = QStringLiteral("droplet");
    valve.access = QStringLiteral("Read");
    valve.simulation.function = SimulationFunction::Square;
    valve.simulation.intervalMs = 50;
    QVERIFY(controller.updateVariable(valve));

    QCOMPARE(structure.count(), 0);
    QCOMPARE(variable.count(), 1);
    QCOMPARE(variable.first().first().value<VariableInfo>().access, QStringLiteral("Read"));
    QCOMPARE(variable.first().first().value<VariableInfo>().icon, QStringLiteral("droplet"));
    QCOMPARE(ProjectNodes::find(controller.project().nodes, valve.nodeId)->icon, QStringLiteral("droplet"));
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

    level.arrayLength = 2;
    QVERIFY(controller.updateVariable(level));
    QCOMPARE(structure.count(), 2);
    QCOMPARE(controller.addressSpace().value(level.nodeId).toList().size(), 2);
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

void TestServerController::updateVariableAppliesInitialValueAndQuality()
{
    ServerController controller;
    controller.setProject(project());
    QSignalSpy structure(&controller, &ServerController::projectChanged);
    const QString nodeId = QStringLiteral("ns=1;s=Tank_01.Level");

    VariableInfo level = SnapshotBuilder::variableInfo(*ProjectNodes::find(controller.project().nodes, nodeId),
                                                       controller.addressSpace());
    level.simulation.function = SimulationFunction::None;
    level.initialValue = QStringLiteral("75.5");
    level.configuredQuality = VariableInfo::Quality::Uncertain;
    QVERIFY(controller.updateVariable(level));

    QCOMPARE(structure.count(), 0);
    const NodeDefinition *stored = ProjectNodes::find(controller.project().nodes, nodeId);
    QCOMPARE(stored->initialValue, QVariant(75.5));
    QCOMPARE(stored->quality, VariableInfo::Quality::Uncertain);
    QCOMPARE(controller.addressSpace().value(nodeId), QVariant(75.5));
    QCOMPARE(controller.addressSpace().quality(nodeId), VariableInfo::Quality::Uncertain);

    level.initialValue = QVariant();
    level.configuredQuality = VariableInfo::Quality::Good;
    QVERIFY(controller.updateVariable(level));
    QCOMPARE(ProjectNodes::find(controller.project().nodes, nodeId)->initialValue, QVariant(75.5));
    QCOMPARE(controller.addressSpace().quality(nodeId), VariableInfo::Quality::Good);
}

void TestServerController::updateVariableRejectsInvalidInitialValue()
{
    ServerController controller;
    controller.setProject(project());
    const QString nodeId = QStringLiteral("ns=1;s=Tank_01.Level");
    VariableInfo level = SnapshotBuilder::variableInfo(*ProjectNodes::find(controller.project().nodes, nodeId),
                                                       controller.addressSpace());
    level.dataType = QStringLiteral("Int16");
    level.initialValue = QStringLiteral("40000");
    QString error;
    QVERIFY(!controller.updateVariable(level, &error));
    QCOMPARE(error, QStringLiteral("'40000' is not a Int16 value"));
    QCOMPARE(ProjectNodes::find(controller.project().nodes, nodeId)->dataType, QStringLiteral("Double"));
}

void TestServerController::renameNodeRebuilds()
{
    ServerController controller;
    controller.setProject(project());
    QSignalSpy structure(&controller, &ServerController::projectChanged);
    QSignalSpy edited(&controller, &ServerController::projectEdited);

    QVERIFY(controller.renameNode(QStringLiteral("ns=1;s=Motor_01"), QStringLiteral("Drive")));
    QCOMPARE(structure.count(), 1);
    QCOMPARE(edited.count(), 1);
    QCOMPARE(ProjectNodes::find(controller.project().nodes, QStringLiteral("ns=1;s=Motor_01"))->browseName,
             QStringLiteral("Drive"));
    QVERIFY(controller.addressSpace().hasVariable(QStringLiteral("ns=1;s=Motor_01.Speed")));

    QString error;
    QVERIFY(!controller.renameNode(QStringLiteral("ns=1;s=Motor_01"), QStringLiteral("Pump_01"), &error));
    QCOMPARE(error, QStringLiteral("A sibling is already named 'Pump_01'"));
    QCOMPARE(structure.count(), 1);
}

void TestServerController::setServerSettingsRequiresRestartWhileRunning()
{
    ServerController controller;
    controller.setProject(project());
    QSignalSpy edited(&controller, &ServerController::projectEdited);

    ServerSettings settings = controller.project().server;
    settings.applicationName = QStringLiteral("Line Simulator");
    settings.maxSessions = 5;
    QVERIFY(controller.setServerSettings(settings));
    QCOMPARE(edited.count(), 1);
    QCOMPARE(controller.project().server.applicationName, QStringLiteral("Line Simulator"));
    QVERIFY(!controller.isRestartRequired());

    QVERIFY(controller.start());
    settings.applicationUri = QStringLiteral("urn:test:line");
    QVERIFY(controller.setServerSettings(settings));
    QVERIFY(controller.isRestartRequired());
    QCOMPARE(controller.status().applicationUri, QStringLiteral("urn:test:line"));
}

void TestServerController::setServerSettingsRejectsInvalidValues_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("uri");
    QTest::addColumn<int>("maxSessions");
    QTest::addColumn<QString>("error");

    QTest::newRow("name") << " " << "urn:x" << 1 << "The application name is empty";
    QTest::newRow("uri") << "Sim" << "" << 1 << "The application URI is empty";
    QTest::newRow("zero sessions") << "Sim" << "urn:x" << 0 << "The session limit must be between 1 and 65535";
    QTest::newRow("too many") << "Sim" << "urn:x" << 65536 << "The session limit must be between 1 and 65535";
}

void TestServerController::setServerSettingsRejectsInvalidValues()
{
    QFETCH(QString, name);
    QFETCH(QString, uri);
    QFETCH(int, maxSessions);
    QFETCH(QString, error);

    ServerController controller;
    controller.setProject(project());
    QSignalSpy edited(&controller, &ServerController::projectEdited);
    QString actual;
    QVERIFY(!controller.setServerSettings({ name, uri, maxSessions }, &actual));
    QCOMPARE(actual, error);
    QCOMPARE(edited.count(), 0);
    QCOMPARE(controller.project().server.applicationName, QStringLiteral("OpenUaSim"));
}

void TestServerController::setUsersValidatesAndRequiresRestart()
{
    ServerController controller;
    controller.setProject(project());
    QSignalSpy edited(&controller, &ServerController::projectEdited);

    QString error;
    QVERIFY(!controller.setUsers({ { QStringLiteral("op"), QString() }, { QStringLiteral("op"), QString() } }, &error));
    QCOMPARE(error, QStringLiteral("The user name 'op' is used more than once"));
    QVERIFY(!controller.setUsers({ { QStringLiteral(" "), QString() } }, &error));
    QCOMPARE(error, QStringLiteral("A user account has no user name"));
    QCOMPARE(edited.count(), 0);

    QVERIFY(controller.setUsers({ { QStringLiteral("op"), QStringLiteral("pw") } }));
    QCOMPARE(edited.count(), 1);
    QCOMPARE(controller.project().users.size(), 1);
    QVERIFY(!controller.isRestartRequired());

    QVERIFY(controller.start());
    QVERIFY(controller.setUsers({}));
    QVERIFY(controller.isRestartRequired());
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
