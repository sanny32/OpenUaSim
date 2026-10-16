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
};

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
