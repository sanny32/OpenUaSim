// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QSignalSpy>
#include <QTest>

#include <QUaProperty>
#include <QUaServer>

#include "server/addressspace.h"
#include "server/simserver.h"
#include "testport.h"
#include "testproject.h"
#include "uatestclient.h"

namespace {

NodeDefinition variable(const QString &name, const QString &dataType, const QString &access = QStringLiteral("Read/Write"))
{
    NodeDefinition node;
    node.kind = NodeDefinition::Kind::Variable;
    node.browseName = name;
    node.nodeId = QStringLiteral("ns=1;s=Test.") + name;
    node.dataType = dataType;
    node.access = access;
    return node;
}

NodeDefinition folder(const QString &name, const QList<NodeDefinition> &children)
{
    NodeDefinition node;
    node.kind = NodeDefinition::Kind::Folder;
    node.browseName = name;
    node.nodeId = QStringLiteral("ns=1;s=") + name;
    node.children = children;
    return node;
}

ProjectConfig serverProject(quint16 port)
{
    ProjectConfig project = TestProject::create();
    project.endpoints.first().port = port;
    return project;
}

}

class TestAddressSpace : public QObject
{
    Q_OBJECT

private slots:
    void buildsDefaultProject();
    void appliesDataTypeAndInitialValue();
    void convertsIncompatibleInitialValueToDefault();
    void setsValueKeepingType();
    void unknownVariable();
    void skipsInvalidNodes_data();
    void skipsInvalidNodes();
    void skipsDuplicateSiblingName();
    void rebuildReplacesNodes();
    void clearRemovesNodesFromServer();
    void clientReadsAndWrites();
    void readOnlyVariableRejectsClientWrite();
    void appliesConfiguredQuality();
    void announcesLiveQualityChanges();
    void buildsArrayVariables();
    void clientCallsMethods();
    void skipsTopLevelMethods();
    void raisesLevelAlarm();
    void raisesOffNormalAlarmFromStart();
    void skipsAlarmWithTakenName();
    void clientReadsBadQuality();
    void clientReadsSpecificQuality();
    void mapsStatusCodesToQualities();
    void clientReadsHistoryWithinLimits();
    void setHistorizingStopsAndResumesRecording();
    void publishesVariableIcons();
    void clientReadsVariableIcon();
};

void TestAddressSpace::buildsDefaultProject()
{
    QUaServer server;
    AddressSpace space(&server);
    const QStringList problems = space.build(TestProject::create().nodes);
    QVERIFY2(problems.isEmpty(), qPrintable(problems.join(QLatin1Char('\n'))));
    QCOMPARE(space.nodeCount(), 17);
    QVERIFY(space.hasVariable(QStringLiteral("ns=1;s=Motor_01.Speed")));
    QCOMPARE(space.value(QStringLiteral("ns=1;s=Motor_01.Speed")).toDouble(), 1000.0);
    QCOMPARE(space.value(QStringLiteral("ns=1;s=Motor_01.Running")), QVariant(true));
    QCOMPARE(space.quality(QStringLiteral("ns=1;s=Motor_01.Speed")), VariableInfo::Quality::Good);
    QVERIFY(server.isNodeIdUsed(QUaNodeId(QStringLiteral("ns=1;s=FactoryLine_01"))));
}

void TestAddressSpace::appliesDataTypeAndInitialValue()
{
    QUaServer server;
    AddressSpace space(&server);
    NodeDefinition counter = variable(QStringLiteral("Counter"), QStringLiteral("UInt16"));
    counter.initialValue = 42;
    QVERIFY(space.build({ folder(QStringLiteral("Test"), { counter }) }).isEmpty());

    const QVariant value = space.value(counter.nodeId);
    QCOMPARE(value.typeId(), int(QMetaType::UShort));
    QCOMPARE(value.toInt(), 42);
}

void TestAddressSpace::convertsIncompatibleInitialValueToDefault()
{
    QUaServer server;
    AddressSpace space(&server);
    NodeDefinition stamp = variable(QStringLiteral("Stamp"), QStringLiteral("DateTime"));
    stamp.initialValue = QStringLiteral("not a date");
    QVERIFY(space.build({ folder(QStringLiteral("Test"), { stamp }) }).isEmpty());
    QCOMPARE(space.value(stamp.nodeId).typeId(), int(QMetaType::QDateTime));
}

void TestAddressSpace::setsValueKeepingType()
{
    QUaServer server;
    AddressSpace space(&server);
    const NodeDefinition level = variable(QStringLiteral("Level"), QStringLiteral("Int32"));
    QVERIFY(space.build({ folder(QStringLiteral("Test"), { level }) }).isEmpty());

    QSignalSpy written(&space, &AddressSpace::valueWrittenByClient);
    QVERIFY(space.setValue(level.nodeId, 12.7));
    const QVariant value = space.value(level.nodeId);
    QCOMPARE(value.typeId(), int(QMetaType::Int));
    QCOMPARE(value.toInt(), 13);
    QCOMPARE(written.count(), 0);
}

void TestAddressSpace::unknownVariable()
{
    QUaServer server;
    AddressSpace space(&server);
    QVERIFY(!space.hasVariable(QStringLiteral("ns=1;s=None")));
    QVERIFY(!space.value(QStringLiteral("ns=1;s=None")).isValid());
    QCOMPARE(space.quality(QStringLiteral("ns=1;s=None")), VariableInfo::Quality::Bad);
    QVERIFY(!space.setValue(QStringLiteral("ns=1;s=None"), 1));
}

void TestAddressSpace::skipsInvalidNodes_data()
{
    QTest::addColumn<QString>("nodeId");
    QTest::addColumn<QString>("dataType");
    QTest::addColumn<QString>("problem");

    QTest::newRow("empty id") << QString() << "Double" << "node id is empty";
    QTest::newRow("malformed id") << "garbage" << "Double" << "node id 'garbage' is malformed";
    QTest::newRow("unknown namespace") << "ns=9;s=X" << "Double" << "namespace 9 of node id 'ns=9;s=X' is not registered";
    QTest::newRow("used id") << "i=85" << "Double" << "node id 'i=85' is already used";
    QTest::newRow("data type") << "ns=1;s=X" << "Guid" << "unsupported data type 'Guid'";
}

void TestAddressSpace::skipsInvalidNodes()
{
    QFETCH(QString, nodeId);
    QFETCH(QString, dataType);
    QFETCH(QString, problem);

    QUaServer server;
    AddressSpace space(&server);
    NodeDefinition bad = variable(QStringLiteral("Bad"), dataType);
    bad.nodeId = nodeId;
    const NodeDefinition good = variable(QStringLiteral("Good"), QStringLiteral("Double"));

    const QStringList problems = space.build({ folder(QStringLiteral("Test"), { bad, good }) });
    QCOMPARE(problems, QStringList { QStringLiteral("Test/Bad: ") + problem });
    QVERIFY(space.hasVariable(good.nodeId));
    QCOMPARE(space.nodeCount(), 2);
}

void TestAddressSpace::skipsDuplicateSiblingName()
{
    QUaServer server;
    AddressSpace space(&server);
    NodeDefinition first = variable(QStringLiteral("Same"), QStringLiteral("Double"));
    NodeDefinition second = first;
    second.nodeId = QStringLiteral("ns=1;s=Other");

    const QStringList problems = space.build({ folder(QStringLiteral("Test"), { first, second }) });
    QCOMPARE(problems, QStringList { QStringLiteral("Test/Same: browse name 'Same' is used by a sibling") });
}

void TestAddressSpace::rebuildReplacesNodes()
{
    QUaServer server;
    AddressSpace space(&server);
    QVERIFY(space.build(TestProject::create().nodes).isEmpty());
    QVERIFY(space.build(TestProject::create().nodes).isEmpty());
    QCOMPARE(space.nodeCount(), 17);

    const NodeDefinition only = variable(QStringLiteral("Only"), QStringLiteral("Double"));
    QVERIFY(space.build({ folder(QStringLiteral("Test"), { only }) }).isEmpty());
    QVERIFY(!space.hasVariable(QStringLiteral("ns=1;s=Motor_01.Speed")));
    QVERIFY(!server.isNodeIdUsed(QUaNodeId(QStringLiteral("ns=1;s=FactoryLine_01"))));
}

void TestAddressSpace::clearRemovesNodesFromServer()
{
    QUaServer server;
    {
        AddressSpace space(&server);
        QVERIFY(space.build(TestProject::create().nodes).isEmpty());
        space.clear();
        QCOMPARE(space.nodeCount(), 0);
        QVERIFY(!server.isNodeIdUsed(QUaNodeId(QStringLiteral("ns=1;s=Motor_01"))));
        QVERIFY(space.build(TestProject::create().nodes).isEmpty());
    }
    QVERIFY(!server.isNodeIdUsed(QUaNodeId(QStringLiteral("ns=1;s=Motor_01"))));
}

void TestAddressSpace::clientReadsAndWrites()
{
    SimServer server;
    AddressSpace space(server.uaServer());
    QVERIFY(space.build(TestProject::create().nodes).isEmpty());
    QVERIFY(server.start(serverProject(TestPort::freePort())));

    UaTestClient client;
    QCOMPARE(client.connect(server.endpointUrl()), UA_STATUSCODE_GOOD);
    QVariant value;
    QCOMPARE(client.readValue(QStringLiteral("ns=1;s=Tank_01.Level"), value), UA_STATUSCODE_GOOD);
    QCOMPARE(value.toDouble(), 50.0);

    QSignalSpy written(&space, &AddressSpace::valueWrittenByClient);
    QCOMPARE(client.writeValue(QStringLiteral("ns=1;s=Tank_01.Level"), 61.5), UA_STATUSCODE_GOOD);
    QTRY_COMPARE(written.count(), 1);
    QCOMPARE(written.first().at(0).toString(), QStringLiteral("ns=1;s=Tank_01.Level"));
    QCOMPARE(written.first().at(1).toDouble(), 61.5);
    QCOMPARE(space.value(QStringLiteral("ns=1;s=Tank_01.Level")).toDouble(), 61.5);
}

void TestAddressSpace::readOnlyVariableRejectsClientWrite()
{
    ProjectConfig project = serverProject(TestPort::freePort());
    const NodeDefinition fixed = variable(QStringLiteral("Fixed"), QStringLiteral("Double"), QStringLiteral("Read"));
    project.nodes = { folder(QStringLiteral("Test"), { fixed }) };

    SimServer server;
    AddressSpace space(server.uaServer());
    QVERIFY(space.build(project.nodes).isEmpty());
    QVERIFY(server.start(project));

    UaTestClient client;
    QCOMPARE(client.connect(server.endpointUrl()), UA_STATUSCODE_GOOD);
    QVERIFY(client.writeValue(fixed.nodeId, 1.0) != UA_STATUSCODE_GOOD);
    QVariant value;
    QCOMPARE(client.readValue(fixed.nodeId, value), UA_STATUSCODE_GOOD);
    QCOMPARE(value.toDouble(), 0.0);
}

void TestAddressSpace::appliesConfiguredQuality()
{
    QUaServer server;
    AddressSpace space(&server);
    NodeDefinition level = variable(QStringLiteral("Level"), QStringLiteral("Double"));
    level.quality = VariableInfo::Quality::Uncertain;
    QVERIFY(space.build({ folder(QStringLiteral("Test"), { level }) }).isEmpty());
    QCOMPARE(space.quality(level.nodeId), VariableInfo::Quality::Uncertain);

    QVERIFY(space.setQuality(level.nodeId, VariableInfo::Quality::Bad));
    QCOMPARE(space.quality(level.nodeId), VariableInfo::Quality::Bad);
    QVERIFY(space.setValue(level.nodeId, 3.0));
    QCOMPARE(space.quality(level.nodeId), VariableInfo::Quality::Bad);

    QVERIFY(space.setQuality(level.nodeId, VariableInfo::Quality::UncertainSensorNotAccurate));
    QCOMPARE(space.quality(level.nodeId), VariableInfo::Quality::UncertainSensorNotAccurate);

    QVERIFY(space.setQuality(level.nodeId, VariableInfo::Quality::Good));
    QVERIFY(space.setValue(level.nodeId, 4.0));
    QCOMPARE(space.quality(level.nodeId), VariableInfo::Quality::Good);
    QVERIFY(!space.setQuality(QStringLiteral("ns=1;s=None"), VariableInfo::Quality::Bad));
}

///
/// \brief Live quality notifications follow status changes and survive rebuilding the same node id.
///
void TestAddressSpace::announcesLiveQualityChanges()
{
    QUaServer server;
    AddressSpace space(&server);
    NodeDefinition level = variable(QStringLiteral("Level"), QStringLiteral("Double"));
    level.initialValue = 7.5;
    level.quality = VariableInfo::Quality::BadWaitingForInitialData;
    QVERIFY(space.build({ level }).isEmpty());
    QSignalSpy changed(&space, &AddressSpace::qualityChanged);
    QVERIFY(space.setValue(level.nodeId, 7.5));
    QCOMPARE(changed.count(), 0);
    QVERIFY(space.setQuality(level.nodeId, VariableInfo::Quality::Good));
    QCOMPARE(changed.count(), 1);
    QCOMPARE(changed.first().at(0).toString(), level.nodeId);
    QCOMPARE(changed.first().at(1).value<VariableInfo::Quality>(), VariableInfo::Quality::Good);
    QVERIFY(space.setValue(level.nodeId, 12.0));
    QCOMPARE(changed.count(), 1);
    QVERIFY(space.setQuality(level.nodeId, VariableInfo::Quality::Uncertain));
    QCOMPARE(changed.count(), 2);
    QCOMPARE(changed.last().at(1).value<VariableInfo::Quality>(), VariableInfo::Quality::Uncertain);

    QVERIFY(space.build({ level }).isEmpty());
    changed.clear();
    QVERIFY(space.setQuality(level.nodeId, VariableInfo::Quality::Good));
    QCOMPARE(changed.count(), 1);
    space.clear();
    QVERIFY(!space.setQuality(level.nodeId, VariableInfo::Quality::Bad));
    QCOMPARE(changed.count(), 1);
}

void TestAddressSpace::buildsArrayVariables()
{
    ProjectConfig project = serverProject(TestPort::freePort());
    NodeDefinition levels = variable(QStringLiteral("Levels"), QStringLiteral("Int32"));
    levels.arrayLength = 3;
    levels.initialValue = 4;
    project.nodes = { folder(QStringLiteral("Test"), { levels }) };

    SimServer server;
    AddressSpace space(server.uaServer());
    QVERIFY(space.build(project.nodes).isEmpty());
    QCOMPARE(space.value(levels.nodeId).toList(), QVariantList({ 4, 4, 4 }));
    auto *built = server.uaServer()->nodeById<QUaBaseDataVariable>(QUaNodeId(levels.nodeId));
    QCOMPARE(built->valueRank(), 1);
    QCOMPARE(built->arrayDimensions(), QVector<quint32>({ 3 }));

    QVERIFY(space.setValue(levels.nodeId, QVariantList { 1.4, 2.6 }));
    QCOMPARE(space.value(levels.nodeId).toList(), QVariantList({ 1, 3, 0 }));

    QVERIFY(server.start(project));
    UaTestClient client;
    QCOMPARE(client.connect(server.endpointUrl()), UA_STATUSCODE_GOOD);
    QVariant value;
    QCOMPARE(client.readValue(levels.nodeId, value), UA_STATUSCODE_GOOD);
    QCOMPARE(value.toList(), QVariantList({ 1, 3, 0 }));
}

void TestAddressSpace::clientCallsMethods()
{
    ProjectConfig project = serverProject(TestPort::freePort());
    NodeDefinition speed = variable(QStringLiteral("Speed"), QStringLiteral("Double"));
    speed.initialValue = 1500.0;
    NodeDefinition state = variable(QStringLiteral("State"), QStringLiteral("String"));
    NodeDefinition reset;
    reset.kind = NodeDefinition::Kind::Method;
    reset.browseName = QStringLiteral("Reset");
    reset.nodeId = QStringLiteral("ns=1;s=Test.Reset");
    NodeDefinition setValue = reset;
    setValue.browseName = QStringLiteral("SetValue");
    setValue.nodeId = QStringLiteral("ns=1;s=Test.SetValue");
    setValue.method = NodeDefinition::MethodAction::SetValue;
    project.nodes = { folder(QStringLiteral("Test"), { speed, state, reset, setValue }) };

    SimServer server;
    AddressSpace space(server.uaServer());
    QVERIFY(space.build(project.nodes).isEmpty());
    QCOMPARE(space.nodeCount(), 5);
    QVERIFY(server.start(project));
    QSignalSpy set(&space, &AddressSpace::valuesSetByMethod);
    QSignalSpy called(&space, &AddressSpace::methodCalled);

    UaTestClient client;
    QCOMPARE(client.connect(server.endpointUrl()), UA_STATUSCODE_GOOD);
    QVariant result;
    const QString object = QStringLiteral("ns=1;s=Test");
    QCOMPARE(client.callMethod(object, setValue.nodeId, { QStringLiteral("Speed"), QStringLiteral("42.5") }, result),
             UA_STATUSCODE_GOOD);
    QCOMPARE(result.toString(), QStringLiteral("Set Speed to 42.5"));
    QCOMPARE(space.value(speed.nodeId).toDouble(), 42.5);
    QCOMPARE(set.count(), 1);

    QCOMPARE(client.callMethod(object, setValue.nodeId, { QStringLiteral("Speed"), QStringLiteral("fast") }, result),
             UA_STATUSCODE_GOOD);
    QCOMPARE(result.toString(), QStringLiteral("'fast' is not a Double value"));
    QCOMPARE(client.callMethod(object, setValue.nodeId, { QStringLiteral("Torque"), QStringLiteral("1") }, result),
             UA_STATUSCODE_GOOD);
    QCOMPARE(result.toString(), QStringLiteral("There is no variable 'Torque'"));
    QCOMPARE(client.callMethod(object, setValue.nodeId, { QStringLiteral("State"), QStringLiteral("run") }, result),
             UA_STATUSCODE_GOOD);
    QCOMPARE(space.value(state.nodeId).toString(), QStringLiteral("run"));

    QCOMPARE(client.callMethod(object, reset.nodeId, {}, result), UA_STATUSCODE_GOOD);
    QCOMPARE(result.toString(), QStringLiteral("Reset 2 variables"));
    QCOMPARE(space.value(speed.nodeId).toDouble(), 1500.0);
    QCOMPARE(space.value(state.nodeId).toString(), QString());
    QCOMPARE(called.count(), 5);
    QCOMPARE(called.last().first().toString(), reset.nodeId);

    QVERIFY(space.build(project.nodes).isEmpty());
    QCOMPARE(client.callMethod(object, reset.nodeId, {}, result), UA_STATUSCODE_GOOD);
}

void TestAddressSpace::skipsTopLevelMethods()
{
    QUaServer server;
    AddressSpace space(&server);
    NodeDefinition method;
    method.kind = NodeDefinition::Kind::Method;
    method.browseName = QStringLiteral("Reset");
    method.nodeId = QStringLiteral("ns=1;s=Reset");
    const QStringList problems = space.build({ method });
    QCOMPARE(problems.size(), 1);
    QVERIFY(problems.first().contains(QStringLiteral("belongs to a folder or object")));
}

void TestAddressSpace::raisesLevelAlarm()
{
    QUaServer server;
    AddressSpace space(&server);
    NodeDefinition level = variable(QStringLiteral("Level"), QStringLiteral("Double"));
    level.initialValue = 50.0;
    level.alarm.enabled = true;
    level.alarm.lowEnabled = true;
    level.alarm.low = 10.0;
    level.alarm.highEnabled = true;
    level.alarm.high = 80.0;
    level.alarm.highHighEnabled = true;
    level.alarm.highHigh = 95.0;
    QVERIFY(space.build({ folder(QStringLiteral("Test"), { level }) }).isEmpty());
    QCOMPARE(space.nodeCount(), 2);
    QVERIFY(server.objectsFolder()->browseChild(QUaQualifiedName(1, QStringLiteral("Test")))->hasChild(QUaQualifiedName(1, QStringLiteral("LevelAlarm"))));
    QVERIFY(space.alarmState(level.nodeId).isEmpty());
    QSignalSpy changed(&space, &AddressSpace::alarmChanged);

    QVERIFY(space.setValue(level.nodeId, 85.0));
    QCOMPARE(space.alarmState(level.nodeId), QStringLiteral("High"));
    QVERIFY(space.setValue(level.nodeId, 97.0));
    QCOMPARE(space.alarmState(level.nodeId), QStringLiteral("HighHigh"));
    QVERIFY(space.setValue(level.nodeId, 50.0));
    QVERIFY(space.alarmState(level.nodeId).isEmpty());
    QCOMPARE(changed.count(), 3);
    QCOMPARE(changed.at(0).at(0).toString(), level.nodeId);
    QCOMPARE(changed.at(0).at(1).toString(), QStringLiteral("High"));
    QCOMPARE(changed.at(2).at(1).toString(), QString());
}

void TestAddressSpace::raisesOffNormalAlarmFromStart()
{
    QUaServer server;
    AddressSpace space(&server);
    NodeDefinition fault = variable(QStringLiteral("Fault"), QStringLiteral("Boolean"));
    fault.initialValue = true;
    fault.alarm.enabled = true;
    fault.alarm.normalValue = false;
    QVERIFY(space.build({ fault }).isEmpty());
    QCOMPARE(space.alarmState(fault.nodeId), QStringLiteral("Active"));
    QVERIFY(space.setValue(fault.nodeId, false));
    QVERIFY(space.alarmState(fault.nodeId).isEmpty());

    space.clear();
    QVERIFY(!server.objectsFolder()->hasChild(QUaQualifiedName(1, QStringLiteral("FaultAlarm"))));
}

void TestAddressSpace::skipsAlarmWithTakenName()
{
    QUaServer server;
    AddressSpace space(&server);
    NodeDefinition level = variable(QStringLiteral("Level"), QStringLiteral("Double"));
    level.alarm.enabled = true;
    level.alarm.highEnabled = true;
    level.alarm.high = 1.0;
    const NodeDefinition taken = variable(QStringLiteral("LevelAlarm"), QStringLiteral("Double"));
    const QStringList problems = space.build({ folder(QStringLiteral("Test"), { taken, level }) });
    QCOMPARE(problems.size(), 1);
    QVERIFY(problems.first().contains(QStringLiteral("alarm name 'LevelAlarm'")));
    QVERIFY(space.hasVariable(level.nodeId));
}

void TestAddressSpace::clientReadsBadQuality()
{
    ProjectConfig project = serverProject(TestPort::freePort());
    NodeDefinition broken = variable(QStringLiteral("Broken"), QStringLiteral("Double"));
    broken.quality = VariableInfo::Quality::Bad;
    project.nodes = { folder(QStringLiteral("Test"), { broken }) };

    SimServer server;
    AddressSpace space(server.uaServer());
    QVERIFY(space.build(project.nodes).isEmpty());
    QVERIFY(server.start(project));

    UaTestClient client;
    QCOMPARE(client.connect(server.endpointUrl()), UA_STATUSCODE_GOOD);
    QVariant value;
    QCOMPARE(client.readValue(broken.nodeId, value), UA_STATUSCODE_BAD);
}

void TestAddressSpace::clientReadsSpecificQuality()
{
    ProjectConfig project = serverProject(TestPort::freePort());
    NodeDefinition failed = variable(QStringLiteral("Failed"), QStringLiteral("Double"));
    failed.quality = VariableInfo::Quality::BadSensorFailure;
    NodeDefinition stale = variable(QStringLiteral("Stale"), QStringLiteral("Double"));
    stale.quality = VariableInfo::Quality::UncertainLastUsableValue;
    project.nodes = { folder(QStringLiteral("Test"), { failed, stale }) };

    SimServer server;
    AddressSpace space(server.uaServer());
    QVERIFY(space.build(project.nodes).isEmpty());
    QVERIFY(server.start(project));

    UaTestClient client;
    QCOMPARE(client.connect(server.endpointUrl()), UA_STATUSCODE_GOOD);
    QVariant value;
    QCOMPARE(client.readValue(failed.nodeId, value), UA_STATUSCODE_BADSENSORFAILURE);
    QCOMPARE(client.readValue(stale.nodeId, value), UA_STATUSCODE_UNCERTAINLASTUSABLEVALUE);
}

void TestAddressSpace::mapsStatusCodesToQualities()
{
    QCOMPARE(VariableInfo::qualityOf(UA_STATUSCODE_GOOD), VariableInfo::Quality::Good);
    QCOMPARE(VariableInfo::qualityOf(UA_STATUSCODE_BADDEVICEFAILURE), VariableInfo::Quality::BadDeviceFailure);
    QCOMPARE(VariableInfo::qualityOf(UA_STATUSCODE_UNCERTAINSUBSTITUTEVALUE | 0x0400),
             VariableInfo::Quality::UncertainSubstituteValue);
    QCOMPARE(VariableInfo::qualityOf(UA_STATUSCODE_UNCERTAINSENSORCALIBRATION), VariableInfo::Quality::Uncertain);
    QCOMPARE(VariableInfo::qualityOf(UA_STATUSCODE_BADTIMEOUT), VariableInfo::Quality::Bad);
    QCOMPARE(VariableInfo::qualityOf(UA_STATUSCODE_GOODCLAMPED), VariableInfo::Quality::Good);
}

void TestAddressSpace::clientReadsHistoryWithinLimits()
{
    ProjectConfig project = serverProject(TestPort::freePort());
    project.server.history.maxValues = 3;
    NodeDefinition recorded = variable(QStringLiteral("Recorded"), QStringLiteral("Double"));
    recorded.historizing = true;
    recorded.initialValue = 1.0;
    const NodeDefinition plain = variable(QStringLiteral("Plain"), QStringLiteral("Double"));
    project.nodes = { folder(QStringLiteral("Test"), { recorded, plain }) };

    SimServer server;
    AddressSpace space(server.uaServer());
    QVERIFY(space.build(project.nodes).isEmpty());
    QVERIFY(server.start(project));

    UaTestClient client;
    QCOMPARE(client.connect(server.endpointUrl()), UA_STATUSCODE_GOOD);
    QVariantList history;
    QCOMPARE(client.readHistory(recorded.nodeId, history), UA_STATUSCODE_GOOD);
    QCOMPARE(history, QVariantList({ QVariant(1.0) }));

    for (double value : { 2.0, 3.0, 4.0, 5.0 }) {
        QTest::qWait(5);
        QVERIFY(space.setValue(recorded.nodeId, value));
        QVERIFY(space.setValue(plain.nodeId, value));
    }
    QCOMPARE(client.readHistory(recorded.nodeId, history), UA_STATUSCODE_GOOD);
    QCOMPARE(history, QVariantList({ 3.0, 4.0, 5.0 }));

    client.readHistory(plain.nodeId, history);
    QVERIFY(history.isEmpty());
}

void TestAddressSpace::setHistorizingStopsAndResumesRecording()
{
    ProjectConfig project = serverProject(TestPort::freePort());
    NodeDefinition level = variable(QStringLiteral("Level"), QStringLiteral("Double"));
    level.initialValue = 1.0;
    project.nodes = { folder(QStringLiteral("Test"), { level }) };

    SimServer server;
    AddressSpace space(server.uaServer());
    QVERIFY(space.build(project.nodes).isEmpty());
    QVERIFY(server.start(project));
    QVERIFY(!space.setHistorizing(QStringLiteral("ns=1;s=None"), true));

    QVERIFY(space.setHistorizing(level.nodeId, true));
    QVERIFY(space.setValue(level.nodeId, 2.0));
    QTest::qWait(5);
    QVERIFY(space.setHistorizing(level.nodeId, false));
    QVERIFY(space.setValue(level.nodeId, 3.0));
    QTest::qWait(5);
    QVERIFY(space.setHistorizing(level.nodeId, true));
    QVERIFY(space.setValue(level.nodeId, 4.0));

    UaTestClient client;
    QCOMPARE(client.connect(server.endpointUrl()), UA_STATUSCODE_GOOD);
    QVariantList history;
    QCOMPARE(client.readHistory(level.nodeId, history), UA_STATUSCODE_GOOD);
    QCOMPARE(history, QVariantList({ 2.0, 4.0 }));
}

void TestAddressSpace::publishesVariableIcons()
{
    QUaServer server;
    AddressSpace space(&server);
    NodeDefinition level = variable(QStringLiteral("Level"), QStringLiteral("Double"));
    level.icon = QStringLiteral("gauge");
    const NodeDefinition plain = variable(QStringLiteral("Plain"), QStringLiteral("Double"));
    const QUaNodeId levelIcon(QStringLiteral("ns=1;s=Test.Level.Icon"));

    QVERIFY(space.build({ folder(QStringLiteral("Test"), { level, plain }) }).isEmpty());
    QVERIFY(!server.nodeById<QUaProperty>(levelIcon));

    space.setIconImages([](const QString &icon) { return icon == u"unknown" ? QByteArray() : icon.toLatin1(); });
    QVERIFY(space.build({ folder(QStringLiteral("Test"), { level, plain }) }).isEmpty());
    QCOMPARE(server.nodeById<QUaProperty>(levelIcon)->value().toByteArray(), QByteArrayLiteral("gauge"));
    QVERIFY(server.nodeById<QUaBaseDataVariable>(QUaNodeId(plain.nodeId))->browseChildren().isEmpty());

    QVERIFY(space.setIcon(level.nodeId, QStringLiteral("thermometer")));
    QCOMPARE(server.nodeById<QUaProperty>(levelIcon)->value().toByteArray(), QByteArrayLiteral("thermometer"));
    QVERIFY(space.setIcon(level.nodeId, QStringLiteral("unknown")));
    QCOMPARE(server.nodeById<QUaProperty>(levelIcon)->value().toByteArray(), QByteArrayLiteral("thermometer"));
    QVERIFY(!space.setIcon(QStringLiteral("ns=1;s=None"), QStringLiteral("gauge")));

    QVERIFY(space.setIcon(level.nodeId, QString()));
    QVERIFY(!server.nodeById<QUaProperty>(levelIcon));
    auto *served = server.nodeById<QUaBaseDataVariable>(QUaNodeId(plain.nodeId));
    QUaProperty *modelIcon = served->addProperty(QUaQualifiedName(0, QStringLiteral("Icon")));
    QVERIFY(space.setIcon(plain.nodeId, QString()));
    QCOMPARE(served->browseChildren(), QList<QUaNode *>({ modelIcon }));
}

void TestAddressSpace::clientReadsVariableIcon()
{
    ProjectConfig project = serverProject(TestPort::freePort());
    NodeDefinition level = variable(QStringLiteral("Level"), QStringLiteral("Double"));
    level.icon = QStringLiteral("gauge");
    project.nodes = { folder(QStringLiteral("Test"), { level }) };
    const QByteArray png = QByteArrayLiteral("\x89PNG\r\n\x1a\n");

    SimServer server;
    AddressSpace space(server.uaServer());
    space.setIconImages([&png](const QString &) { return png; });
    QVERIFY(space.build(project.nodes).isEmpty());
    QVERIFY(server.start(project));

    UaTestClient client;
    QCOMPARE(client.connect(server.endpointUrl()), UA_STATUSCODE_GOOD);
    QVariant value;
    QCOMPARE(client.readValue(QStringLiteral("ns=1;s=Test.Level.Icon"), value), UA_STATUSCODE_GOOD);
    QCOMPARE(value.toByteArray(), png);
    QUaNodeId dataType;
    QCOMPARE(client.readDataType(QStringLiteral("ns=1;s=Test.Level.Icon"), dataType), UA_STATUSCODE_GOOD);
    QCOMPARE(dataType, QUaNodeId(0, quint32(UA_NS0ID_IMAGEPNG)));
}

QTEST_GUILESS_MAIN(TestAddressSpace)

#include "test_addressspace.moc"
