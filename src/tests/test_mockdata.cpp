// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <algorithm>

#include <QSet>
#include <QTest>

#include "mock/mockdata.h"

namespace {

void collect(const QList<TreeNode> &nodes, QList<const TreeNode *> &out)
{
    for (const TreeNode &node : nodes) {
        out.append(&node);
        collect(node.children, out);
    }
}

int countCurrent(const QList<TreeNode> &nodes)
{
    QList<const TreeNode *> all;
    collect(nodes, all);
    return static_cast<int>(std::count_if(all.cbegin(), all.cend(),
                                          [](const TreeNode *node) { return node->current; }));
}

}

class TestMockData : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void statusMatchesContent();
    void everyObjectHasItsVariables();
    void treesMarkOneCurrentNode();
    void typesFormAHierarchy();
    void namespaceIndexesAreSequential();
    void certificatesCoverEveryFolder();
    void logIsChronological();

private:
    ServerSnapshot _snapshot;
};

void TestMockData::initTestCase()
{
    _snapshot = MockData::snapshot();
}

void TestMockData::statusMatchesContent()
{
    const ServerStatus &status = _snapshot.status;
    QVERIFY(status.running);
    QCOMPARE(status.primaryEndpoint, _snapshot.endpoints.first().url());
    QCOMPARE(status.namespaces, _snapshot.namespaces.size());
    const auto active = std::count_if(_snapshot.sessions.cbegin(), _snapshot.sessions.cend(),
                                      [](const SessionInfo &session) { return session.active; });
    QCOMPARE(status.clientsConnected, int(active));

    QList<const TreeNode *> nodes;
    collect(_snapshot.addressSpace, nodes);
    QCOMPARE(status.totalNodes, nodes.size());
}

void TestMockData::everyObjectHasItsVariables()
{
    QList<const TreeNode *> nodes;
    collect(_snapshot.addressSpace, nodes);

    int objects = 0;
    for (const TreeNode *node : std::as_const(nodes)) {
        if (node->kind != TreeNode::Kind::Object)
            continue;
        ++objects;
        QVERIFY2(_snapshot.variables.contains(node->id), qPrintable(node->id));
        const QList<VariableInfo> &variables = _snapshot.variables.value(node->id);
        QCOMPARE(variables.size(), node->children.size());
        for (int i = 0; i < variables.size(); ++i) {
            QCOMPARE(node->children.at(i).id, variables.at(i).nodeId);
            QCOMPARE(node->children.at(i).kind, TreeNode::Kind::Variable);
            QVERIFY(variables.at(i).value.isValid());
            QVERIFY(!variables.at(i).icon.isEmpty());
        }
    }
    QCOMPARE(objects, _snapshot.variables.size());
}

void TestMockData::treesMarkOneCurrentNode()
{
    QCOMPARE(countCurrent(_snapshot.addressSpace), 1);
    QCOMPARE(countCurrent(_snapshot.typeTree), 1);
}

void TestMockData::typesFormAHierarchy()
{
    QList<const TreeNode *> nodes;
    collect(_snapshot.typeTree, nodes);
    for (const TreeNode *node : std::as_const(nodes)) {
        if (node->kind == TreeNode::Kind::Type)
            QVERIFY2(_snapshot.types.contains(node->id), qPrintable(node->id));
    }
    for (const TypeInfo &type : std::as_const(_snapshot.types)) {
        if (!type.baseTypeId.isEmpty())
            QVERIFY2(_snapshot.types.contains(type.baseTypeId), qPrintable(type.baseTypeId));
        QCOMPARE(type.references.first().referenceType, QStringLiteral("HasSubtype"));
    }
    QCOMPARE(_snapshot.types.value(QStringLiteral("ns=2;i=1000")).references.first().count, 3);
}

void TestMockData::namespaceIndexesAreSequential()
{
    for (int i = 0; i < _snapshot.namespaces.size(); ++i)
        QCOMPARE(_snapshot.namespaces.at(i).index, i);
    QCOMPARE(_snapshot.namespaces.first().uri, QStringLiteral("http://opcfoundation.org/UA/"));
}

void TestMockData::certificatesCoverEveryFolder()
{
    QSet<CertificateInfo::Category> categories;
    for (const CertificateInfo &certificate : std::as_const(_snapshot.certificates)) {
        categories.insert(certificate.category);
        QVERIFY(certificate.validFrom < certificate.validTo);
    }
    QCOMPARE(categories.size(), 4);
}

void TestMockData::logIsChronological()
{
    QVERIFY(!_snapshot.log.isEmpty());
    for (int i = 1; i < _snapshot.log.size(); ++i)
        QVERIFY(_snapshot.log.at(i - 1).time <= _snapshot.log.at(i).time);
}

QTEST_GUILESS_MAIN(TestMockData)
#include "test_mockdata.moc"
