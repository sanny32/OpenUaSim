// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QSignalSpy>
#include <QTest>

#include <QUaServer>

#include "project/defaultproject.h"
#include "server/addressspace.h"
#include "server/simserver.h"
#include "testport.h"
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
    ProjectConfig project = DefaultProject::create();
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
    void clientReadsBadQuality();
};

void TestAddressSpace::buildsDefaultProject()
{
    QUaServer server;
    AddressSpace space(&server);
    const QStringList problems = space.build(DefaultProject::create().nodes);
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
    QVERIFY(space.build(DefaultProject::create().nodes).isEmpty());
    QVERIFY(space.build(DefaultProject::create().nodes).isEmpty());
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
        QVERIFY(space.build(DefaultProject::create().nodes).isEmpty());
        space.clear();
        QCOMPARE(space.nodeCount(), 0);
        QVERIFY(!server.isNodeIdUsed(QUaNodeId(QStringLiteral("ns=1;s=Motor_01"))));
        QVERIFY(space.build(DefaultProject::create().nodes).isEmpty());
    }
    QVERIFY(!server.isNodeIdUsed(QUaNodeId(QStringLiteral("ns=1;s=Motor_01"))));
}

void TestAddressSpace::clientReadsAndWrites()
{
    SimServer server;
    AddressSpace space(server.uaServer());
    QVERIFY(space.build(DefaultProject::create().nodes).isEmpty());
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

    QVERIFY(space.setQuality(level.nodeId, VariableInfo::Quality::Good));
    QVERIFY(space.setValue(level.nodeId, 4.0));
    QCOMPARE(space.quality(level.nodeId), VariableInfo::Quality::Good);
    QVERIFY(!space.setQuality(QStringLiteral("ns=1;s=None"), VariableInfo::Quality::Bad));
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

QTEST_GUILESS_MAIN(TestAddressSpace)

#include "test_addressspace.moc"
