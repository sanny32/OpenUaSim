// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QTest>

#include <algorithm>

#include "project/defaultproject.h"
#include "server/addressspace.h"
#include "server/simserver.h"
#include "server/snapshotbuilder.h"
#include "server/typecatalog.h"
#include "testport.h"

namespace {

const TreeNode *findById(const QList<TreeNode> &nodes, const QString &id)
{
    for (const TreeNode &node : nodes) {
        if (node.id == id)
            return &node;
        if (const TreeNode *found = findById(node.children, id))
            return found;
    }
    return nullptr;
}

}

class TestSnapshotBuilder : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void treeHasStandardRootsAndProjectNodes();
    void treeSelectsFirstVariable();
    void treeOfEmptyProject();
    void variablesCarryLiveValues();
    void unbuiltVariablesAreBad();
    void stoppedServerStatus();
    void runningServerStatus();
    void statusOfInvalidEndpoints();
    void listsNamespaces();
    void typeCatalogReadsServerTypes();
    void typeTreeHidesUnusedStandardTypes();
    void buildCombinesEverything();

private:
    ProjectConfig _project;
    SimServer *_server = nullptr;
    AddressSpace *_space = nullptr;
};

void TestSnapshotBuilder::init()
{
    _project = DefaultProject::create();
    _project.endpoints.first().port = TestPort::freePort();
    _server = new SimServer;
    _space = new AddressSpace(_server->uaServer());
    QVERIFY(_space->build(_project.nodes).isEmpty());
}

void TestSnapshotBuilder::cleanup()
{
    delete _space;
    delete _server;
}

void TestSnapshotBuilder::treeHasStandardRootsAndProjectNodes()
{
    const QList<TreeNode> tree = SnapshotBuilder::addressSpaceTree(_project.nodes);
    QCOMPARE(tree.size(), 3);
    QCOMPARE(tree[0].text, QStringLiteral("Objects"));
    QCOMPARE(tree[0].id, QStringLiteral("i=85"));
    QVERIFY(tree[0].expanded);
    QCOMPARE(tree[1].text, QStringLiteral("Types"));
    QCOMPARE(tree[2].text, QStringLiteral("Views"));

    const TreeNode *motor = findById(tree, QStringLiteral("ns=1;s=Motor_01"));
    QVERIFY(motor);
    QCOMPARE(motor->kind, TreeNode::Kind::Object);
    QCOMPARE(motor->children.size(), 3);
    QCOMPARE(motor->children.first().kind, TreeNode::Kind::Variable);
    QCOMPARE(motor->children.first().icon, QStringLiteral("signal"));
}

void TestSnapshotBuilder::treeSelectsFirstVariable()
{
    const QList<TreeNode> tree = SnapshotBuilder::addressSpaceTree(_project.nodes);
    QVERIFY(findById(tree, QStringLiteral("ns=1;s=FactoryLine_01"))->expanded);
    QVERIFY(findById(tree, QStringLiteral("ns=1;s=Motor_01"))->expanded);
    QVERIFY(findById(tree, QStringLiteral("ns=1;s=Motor_01.Speed"))->current);
    QVERIFY(!findById(tree, QStringLiteral("ns=1;s=Pump_01"))->expanded);
}

void TestSnapshotBuilder::treeOfEmptyProject()
{
    const QList<TreeNode> tree = SnapshotBuilder::addressSpaceTree({});
    QCOMPARE(tree.size(), 3);
    QVERIFY(tree[0].children.isEmpty());
}

void TestSnapshotBuilder::variablesCarryLiveValues()
{
    QVERIFY(_space->setValue(QStringLiteral("ns=1;s=Motor_01.Speed"), 1234.5));
    const QHash<QString, QList<VariableInfo>> variables = SnapshotBuilder::variables(_project.nodes, *_space);
    QCOMPARE(variables.size(), 4);

    const QList<VariableInfo> motor = variables.value(QStringLiteral("ns=1;s=Motor_01"));
    QCOMPARE(motor.size(), 3);
    QCOMPARE(motor[0].displayName, QStringLiteral("Speed"));
    QCOMPARE(motor[0].value.toDouble(), 1234.5);
    QCOMPARE(motor[0].quality, VariableInfo::Quality::Good);
    QCOMPARE(motor[0].simulation.function, SimulationFunction::Sine);
    QCOMPARE(motor[2].dataType, QStringLiteral("Boolean"));
}

void TestSnapshotBuilder::unbuiltVariablesAreBad()
{
    AddressSpace empty(_server->uaServer());
    const QHash<QString, QList<VariableInfo>> variables = SnapshotBuilder::variables(_project.nodes, empty);
    const VariableInfo speed = variables.value(QStringLiteral("ns=1;s=Motor_01")).first();
    QCOMPARE(speed.quality, VariableInfo::Quality::Bad);
    QCOMPARE(speed.value.toDouble(), 1000.0);
}

void TestSnapshotBuilder::stoppedServerStatus()
{
    const ServerStatus status = SnapshotBuilder::status(_project, *_server, *_space, QDateTime::currentDateTime());
    QVERIFY(!status.running);
    QCOMPARE(status.primaryEndpoint, _project.endpoints.first().url());
    QCOMPARE(status.applicationUri, QStringLiteral("urn:openuasim:server"));
    QCOMPARE(status.securityModes, QStringList { QStringLiteral("None") });
    QCOMPARE(status.totalNodes, 17);
    QVERIFY(status.namespaces >= 2);
    QCOMPARE(status.uptimeSeconds, 0);
    QVERIFY(!status.startTime.isValid());
}

void TestSnapshotBuilder::runningServerStatus()
{
    QVERIFY(_server->start(_project));
    const ServerStatus status =
        SnapshotBuilder::status(_project, *_server, *_space, _server->startTime().addSecs(75));
    QVERIFY(status.running);
    QCOMPARE(status.startTime, _server->startTime());
    QCOMPARE(status.uptimeSeconds, 75);
    QCOMPARE(status.clientsConnected, 0);
}

void TestSnapshotBuilder::statusOfInvalidEndpoints()
{
    _project.endpoints.clear();
    const ServerStatus status = SnapshotBuilder::status(_project, *_server, *_space, QDateTime::currentDateTime());
    QVERIFY(status.primaryEndpoint.isEmpty());
    QVERIFY(status.securityModes.isEmpty());
}

void TestSnapshotBuilder::listsNamespaces()
{
    const QList<NamespaceInfo> namespaces = SnapshotBuilder::namespaces(*_server);
    QVERIFY(namespaces.size() >= 2);
    QCOMPARE(namespaces[0].index, 0);
    QCOMPARE(namespaces[0].uri, QStringLiteral("http://opcfoundation.org/UA/"));
    QCOMPARE(namespaces[1].index, 1);
    QVERIFY(!namespaces[1].description.isEmpty());
}

void TestSnapshotBuilder::typeCatalogReadsServerTypes()
{
    const QHash<QString, TypeInfo> types = TypeCatalog::types(*_server->uaServer());
    const TypeInfo variableRoot = types.value(QStringLiteral("i=62"));
    QCOMPARE(variableRoot.browseName, QStringLiteral("BaseVariableType"));
    QVERIFY(variableRoot.isAbstract);
    QVERIFY(variableRoot.baseTypeId.isEmpty());
    QCOMPARE(variableRoot.category, QStringLiteral("VariableType"));
    QVERIFY(!variableRoot.description.isEmpty());
    const auto subtypes = std::find_if(
        variableRoot.references.cbegin(), variableRoot.references.cend(),
        [](const ReferenceCount &count) { return count.referenceType == QLatin1String("HasSubtype"); });
    QVERIFY(subtypes != variableRoot.references.cend());
    QVERIFY(subtypes->count >= 2);
    QCOMPARE(types.value(QStringLiteral("i=61")).baseTypeId, QStringLiteral("i=58"));
    QCOMPARE(types.value(QStringLiteral("i=29")).category, QStringLiteral("DataType"));
    QCOMPARE(types.value(QStringLiteral("i=35")).category, QStringLiteral("ReferenceType"));
    QCOMPARE(types.value(QStringLiteral("i=11")).browseName, QStringLiteral("Double"));
}

void TestSnapshotBuilder::typeTreeHidesUnusedStandardTypes()
{
    const QHash<QString, TypeInfo> types = TypeCatalog::types(*_server->uaServer());
    const QList<TreeNode> relevant = TypeCatalog::tree(types, false);
    QCOMPARE(relevant.size(), 1);
    QCOMPARE(relevant.first().children.size(), 4);
    QVERIFY(findById(relevant, QStringLiteral("i=61"))->current);
    QVERIFY(findById(relevant, QStringLiteral("i=11")));
    QVERIFY(findById(relevant, QStringLiteral("i=26")));
    QVERIFY(!findById(relevant, QStringLiteral("i=852")));

    const QList<TreeNode> all = TypeCatalog::tree(types, true);
    for (const QString &id : types.keys())
        QVERIFY2(findById(all, id), qPrintable(id));
}

void TestSnapshotBuilder::buildCombinesEverything()
{
    const ServerSnapshot snapshot = SnapshotBuilder::build(_project, *_server, *_space, QDateTime::currentDateTime());
    QCOMPARE(snapshot.addressSpace.size(), 3);
    QCOMPARE(snapshot.variables.size(), 4);
    QCOMPARE(snapshot.types.value(QStringLiteral("i=61")).browseName, QStringLiteral("FolderType"));
    QCOMPARE(snapshot.endpoints.size(), 1);
    QCOMPARE(snapshot.namespaces.size(), snapshot.status.namespaces);
    QVERIFY(snapshot.sessions.isEmpty());
}

QTEST_GUILESS_MAIN(TestSnapshotBuilder)

#include "test_snapshotbuilder.moc"
