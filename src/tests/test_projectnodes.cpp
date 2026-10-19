// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QTest>

#include "project/defaultproject.h"
#include "project/projectnodes.h"

namespace {

NodeDefinition node(NodeDefinition::Kind kind, const QString &name, const QString &nodeId = QString(),
                    const QList<NodeDefinition> &children = {})
{
    NodeDefinition result;
    result.kind = kind;
    result.browseName = name;
    result.nodeId = nodeId;
    result.children = children;
    return result;
}

}

class TestProjectNodes : public QObject
{
    Q_OBJECT

private slots:
    void assignsIdsFromBrowsePath();
    void keepsExistingIds();
    void avoidsTakenIds();
    void defaultProjectNeedsNoIds();
    void countsEveryDepth();
    void findsNodes();
    void listsIdsAndChildNames();
    void insertsBelowContainer();
    void insertsAtTopLevel();
    void insertRejectsInvalidNodes_data();
    void insertRejectsInvalidNodes();
    void removesSubtree();
    void updatesVariable();
    void updateConvertsInitialValue();
    void updateRejectsInvalidChanges_data();
    void updateRejectsInvalidChanges();
};

void TestProjectNodes::findsNodes()
{
    const QList<NodeDefinition> nodes = DefaultProject::create().nodes;
    const NodeDefinition *speed = ProjectNodes::find(nodes, QStringLiteral("ns=1;s=Pump_01.Flow"));
    QVERIFY(speed);
    QCOMPARE(speed->browseName, QStringLiteral("Flow"));
    QVERIFY(!ProjectNodes::find(nodes, QStringLiteral("ns=1;s=None")));
}

void TestProjectNodes::listsIdsAndChildNames()
{
    const QList<NodeDefinition> nodes = DefaultProject::create().nodes;
    QCOMPARE(ProjectNodes::nodeIds(nodes).size(), 17);
    QCOMPARE(ProjectNodes::childNames(nodes, QString()), QStringList { QStringLiteral("FactoryLine_01") });
    QCOMPARE(ProjectNodes::childNames(nodes, QStringLiteral("ns=1;s=Tank_01")),
             QStringList({ QStringLiteral("Level"), QStringLiteral("Temperature"), QStringLiteral("InletValve") }));
    QVERIFY(ProjectNodes::childNames(nodes, QStringLiteral("ns=1;s=None")).isEmpty());
}

void TestProjectNodes::insertsBelowContainer()
{
    QList<NodeDefinition> nodes = DefaultProject::create().nodes;
    const QString id = ProjectNodes::insert(nodes, QStringLiteral("ns=1;s=Tank_01"),
                                            node(NodeDefinition::Kind::Variable, QStringLiteral("Outlet")));
    QCOMPARE(id, QStringLiteral("ns=1;s=FactoryLine_01.Tank_01.Outlet"));
    QCOMPARE(ProjectNodes::find(nodes, QStringLiteral("ns=1;s=Tank_01"))->children.last().browseName,
             QStringLiteral("Outlet"));
}

void TestProjectNodes::insertsAtTopLevel()
{
    QList<NodeDefinition> nodes;
    QCOMPARE(ProjectNodes::insert(nodes, QString(), node(NodeDefinition::Kind::Folder, QStringLiteral("Line"),
                                                         QStringLiteral("i=7000"))),
             QStringLiteral("i=7000"));
    QCOMPARE(nodes.size(), 1);
}

void TestProjectNodes::insertRejectsInvalidNodes_data()
{
    QTest::addColumn<QString>("parentId");
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("nodeId");
    QTest::addColumn<QString>("dataType");
    QTest::addColumn<QString>("error");

    QTest::newRow("unknown parent") << "ns=1;s=None" << "X" << "" << "Double" << "'ns=1;s=None' cannot hold child nodes";
    QTest::newRow("variable parent") << "ns=1;s=Tank_01.Level" << "X" << "" << "Double"
                                     << "'ns=1;s=Tank_01.Level' cannot hold child nodes";
    QTest::newRow("empty name") << "ns=1;s=Tank_01" << " " << "" << "Double" << "The browse name is empty";
    QTest::newRow("sibling name") << "ns=1;s=Tank_01" << "Level" << "" << "Double"
                                  << "A sibling is already named 'Level'";
    QTest::newRow("used id") << "ns=1;s=Tank_01" << "X" << "ns=1;s=Motor_01" << "Double"
                             << "The node id 'ns=1;s=Motor_01' is already used";
    QTest::newRow("data type") << "ns=1;s=Tank_01" << "X" << "" << "Guid" << "Unsupported data type 'Guid'";
}

void TestProjectNodes::insertRejectsInvalidNodes()
{
    QFETCH(QString, parentId);
    QFETCH(QString, name);
    QFETCH(QString, nodeId);
    QFETCH(QString, dataType);
    QFETCH(QString, error);

    QList<NodeDefinition> nodes = DefaultProject::create().nodes;
    NodeDefinition added = node(NodeDefinition::Kind::Variable, name, nodeId);
    added.dataType = dataType;
    QString actual;
    QVERIFY(ProjectNodes::insert(nodes, parentId, added, &actual).isEmpty());
    QCOMPARE(actual, error);
    QCOMPARE(ProjectNodes::count(nodes), 17);
}

void TestProjectNodes::removesSubtree()
{
    QList<NodeDefinition> nodes = DefaultProject::create().nodes;
    QVERIFY(ProjectNodes::remove(nodes, QStringLiteral("ns=1;s=Pump_01")));
    QCOMPARE(ProjectNodes::count(nodes), 13);
    QVERIFY(!ProjectNodes::find(nodes, QStringLiteral("ns=1;s=Pump_01.Flow")));
    QVERIFY(!ProjectNodes::remove(nodes, QStringLiteral("ns=1;s=Pump_01")));
    QVERIFY(ProjectNodes::remove(nodes, QStringLiteral("ns=1;s=FactoryLine_01")));
    QVERIFY(nodes.isEmpty());
}

void TestProjectNodes::updatesVariable()
{
    QList<NodeDefinition> nodes = DefaultProject::create().nodes;
    NodeDefinition edited = *ProjectNodes::find(nodes, QStringLiteral("ns=1;s=Motor_01.Speed"));
    edited.browseName = QStringLiteral("Rpm");
    edited.icon = QStringLiteral("zap");
    edited.access = QStringLiteral("Read");
    edited.simulation.function = SimulationFunction::Step;
    edited.children = { node(NodeDefinition::Kind::Variable, QStringLiteral("Ignored")) };

    QVERIFY(ProjectNodes::updateVariable(nodes, edited));
    const NodeDefinition *stored = ProjectNodes::find(nodes, edited.nodeId);
    QCOMPARE(stored->browseName, QStringLiteral("Rpm"));
    QCOMPARE(stored->icon, QStringLiteral("zap"));
    QCOMPARE(stored->access, QStringLiteral("Read"));
    QCOMPARE(stored->simulation.function, SimulationFunction::Step);
    QVERIFY(stored->children.isEmpty());
}

void TestProjectNodes::updateConvertsInitialValue()
{
    QList<NodeDefinition> nodes = DefaultProject::create().nodes;
    NodeDefinition edited = *ProjectNodes::find(nodes, QStringLiteral("ns=1;s=Motor_01.Speed"));
    edited.dataType = QStringLiteral("Int32");
    QVERIFY(ProjectNodes::updateVariable(nodes, edited));
    const QVariant value = ProjectNodes::find(nodes, edited.nodeId)->initialValue;
    QCOMPARE(value.typeId(), int(QMetaType::Int));
    QCOMPARE(value.toInt(), 1000);
}

void TestProjectNodes::updateRejectsInvalidChanges_data()
{
    QTest::addColumn<QString>("nodeId");
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("dataType");
    QTest::addColumn<QString>("error");

    QTest::newRow("unknown") << "ns=1;s=None" << "X" << "Double" << "'ns=1;s=None' is not a variable";
    QTest::newRow("object") << "ns=1;s=Motor_01" << "X" << "Double" << "'ns=1;s=Motor_01' is not a variable";
    QTest::newRow("empty name") << "ns=1;s=Motor_01.Speed" << "" << "Double" << "The browse name is empty";
    QTest::newRow("sibling") << "ns=1;s=Motor_01.Speed" << "Running" << "Double"
                             << "A sibling is already named 'Running'";
    QTest::newRow("type") << "ns=1;s=Motor_01.Speed" << "Speed" << "Guid" << "Unsupported data type 'Guid'";
}

void TestProjectNodes::updateRejectsInvalidChanges()
{
    QFETCH(QString, nodeId);
    QFETCH(QString, name);
    QFETCH(QString, dataType);
    QFETCH(QString, error);

    QList<NodeDefinition> nodes = DefaultProject::create().nodes;
    NodeDefinition edited = node(NodeDefinition::Kind::Variable, name, nodeId);
    edited.dataType = dataType;
    QString actual;
    QVERIFY(!ProjectNodes::updateVariable(nodes, edited, &actual));
    QCOMPARE(actual, error);
    QCOMPARE(ProjectNodes::find(nodes, QStringLiteral("ns=1;s=Motor_01.Speed"))->browseName, QStringLiteral("Speed"));
}

void TestProjectNodes::assignsIdsFromBrowsePath()
{
    using Kind = NodeDefinition::Kind;
    QList<NodeDefinition> nodes = {
        node(Kind::Folder, QStringLiteral("Line"), QString(),
             { node(Kind::Object, QStringLiteral("Motor"), QString(), { node(Kind::Variable, QStringLiteral("Speed")) }) }),
    };
    QCOMPARE(ProjectNodes::assignMissingNodeIds(nodes), 3);
    QCOMPARE(nodes[0].nodeId, QStringLiteral("ns=1;s=Line"));
    QCOMPARE(nodes[0].children[0].nodeId, QStringLiteral("ns=1;s=Line.Motor"));
    QCOMPARE(nodes[0].children[0].children[0].nodeId, QStringLiteral("ns=1;s=Line.Motor.Speed"));
}

void TestProjectNodes::keepsExistingIds()
{
    QList<NodeDefinition> nodes = { node(NodeDefinition::Kind::Folder, QStringLiteral("Line"), QStringLiteral("i=5000")) };
    QCOMPARE(ProjectNodes::assignMissingNodeIds(nodes), 0);
    QCOMPARE(nodes[0].nodeId, QStringLiteral("i=5000"));
}

void TestProjectNodes::avoidsTakenIds()
{
    using Kind = NodeDefinition::Kind;
    QList<NodeDefinition> nodes = {
        node(Kind::Variable, QStringLiteral("Other"), QStringLiteral("ns=1;s=Line")),
        node(Kind::Folder, QStringLiteral("Line")),
        node(Kind::Variable, QStringLiteral("Taken"), QStringLiteral("ns=1;s=Line_2")),
    };
    QCOMPARE(ProjectNodes::assignMissingNodeIds(nodes), 1);
    QCOMPARE(nodes[1].nodeId, QStringLiteral("ns=1;s=Line_3"));
}

void TestProjectNodes::defaultProjectNeedsNoIds()
{
    ProjectConfig project = DefaultProject::create();
    QCOMPARE(ProjectNodes::assignMissingNodeIds(project.nodes), 0);
}

void TestProjectNodes::countsEveryDepth()
{
    QCOMPARE(ProjectNodes::count({}), 0);
    QCOMPARE(ProjectNodes::count(DefaultProject::create().nodes), 1 + 4 + 12);
}

QTEST_GUILESS_MAIN(TestProjectNodes)

#include "test_projectnodes.moc"
