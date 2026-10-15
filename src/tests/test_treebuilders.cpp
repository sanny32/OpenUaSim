// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QTest>

#include "treebuilders.h"

namespace {

SessionInfo session(const QString &id, const QString &name, bool active)
{
    SessionInfo result;
    result.sessionId = id;
    result.name = name;
    result.active = active;
    return result;
}

CertificateInfo certificate(CertificateInfo::Category category)
{
    CertificateInfo result;
    result.category = category;
    return result;
}

}

class TestTreeBuilders : public QObject
{
    Q_OBJECT

private slots:
    void namespaceTree();
    void namespaceTreeWithoutNamespaces();
    void endpointList();
    void certificateCategories();
    void sessionTree();
    void sessionTreeWithoutSessions();
};

void TestTreeBuilders::namespaceTree()
{
    const QList<TreeNode> tree = TreeBuilders::namespaceTree({
        { 0, QStringLiteral("http://opcfoundation.org/UA/"), {}, {}, {} },
        { 2, QStringLiteral("urn:factory"), {}, {}, {} },
    });

    QCOMPARE(tree.size(), 1);
    QCOMPARE(tree.first().text, QStringLiteral("All Namespaces"));
    QVERIFY(tree.first().expanded);
    QCOMPARE(tree.first().children.size(), 2);
    const TreeNode &factory = tree.first().children.at(1);
    QCOMPARE(factory.text, QStringLiteral("[2] urn:factory"));
    QCOMPARE(factory.id, QStringLiteral("urn:factory"));
    QCOMPARE(factory.kind, TreeNode::Kind::Namespace);
}

void TestTreeBuilders::namespaceTreeWithoutNamespaces()
{
    const QList<TreeNode> tree = TreeBuilders::namespaceTree({});
    QCOMPARE(tree.size(), 1);
    QVERIFY(tree.first().children.isEmpty());
}

void TestTreeBuilders::endpointList()
{
    EndpointInfo endpoint;
    endpoint.name = QStringLiteral("Discovery");
    endpoint.icon = QStringLiteral("globe");
    endpoint.protocol = QStringLiteral("opc.tcp");
    endpoint.hostname = QStringLiteral("localhost");
    endpoint.path = QStringLiteral("/discovery");

    const QList<TreeNode> list = TreeBuilders::endpointList({ endpoint });
    QCOMPARE(list.size(), 1);
    QCOMPARE(list.first().text, QStringLiteral("Discovery\nopc.tcp://localhost:4840/discovery"));
    QCOMPARE(list.first().id, endpoint.url());
    QCOMPARE(list.first().icon, QStringLiteral("globe"));
    QCOMPARE(list.first().kind, TreeNode::Kind::Endpoint);
    QVERIFY(TreeBuilders::endpointList({}).isEmpty());
}

void TestTreeBuilders::certificateCategories()
{
    using Category = CertificateInfo::Category;
    const QList<TreeNode> nodes = TreeBuilders::certificateCategories({
        certificate(Category::Trusted), certificate(Category::Trusted),
        certificate(Category::Rejected), certificate(Category::Trusted),
    });

    QCOMPARE(nodes.size(), 4);
    QCOMPARE(nodes.at(0).text, QStringLiteral("Application Certificate"));
    QCOMPARE(nodes.at(0).trailing, QStringLiteral("0"));
    QCOMPARE(nodes.at(1).text, QStringLiteral("Trusted Clients"));
    QCOMPARE(nodes.at(1).trailing, QStringLiteral("3"));
    QCOMPARE(nodes.at(1).id, QString::number(static_cast<int>(Category::Trusted)));
    QCOMPARE(nodes.at(2).trailing, QStringLiteral("1"));
    QCOMPARE(nodes.at(3).text, QStringLiteral("Issuers"));
    for (const TreeNode &node : nodes)
        QVERIFY(!node.icon.isEmpty());
}

void TestTreeBuilders::sessionTree()
{
    const QList<TreeNode> tree = TreeBuilders::sessionTree({
        session(QStringLiteral("ns=1;i=1"), QStringLiteral("UaExpert"), true),
        session(QStringLiteral("ns=1;i=2"), QStringLiteral("QuickClient"), false),
        session(QStringLiteral("ns=1;i=3"), QStringLiteral("Prosys"), false),
    });

    QCOMPARE(tree.size(), 2);
    QCOMPARE(tree.at(0).text, QStringLiteral("Active Sessions (1)"));
    QCOMPARE(tree.at(1).text, QStringLiteral("Closed Sessions (2)"));
    QVERIFY(tree.at(0).expanded);
    QCOMPARE(tree.at(0).children.first().id, QStringLiteral("ns=1;i=1"));
    QCOMPARE(tree.at(0).children.first().kind, TreeNode::Kind::Session);
    QCOMPARE(tree.at(1).children.at(1).text, QStringLiteral("Prosys"));
}

void TestTreeBuilders::sessionTreeWithoutSessions()
{
    const QList<TreeNode> tree = TreeBuilders::sessionTree({});
    QCOMPARE(tree.size(), 2);
    QCOMPARE(tree.at(0).text, QStringLiteral("Active Sessions (0)"));
    QVERIFY(tree.at(0).children.isEmpty());
    QVERIFY(tree.at(1).children.isEmpty());
}

QTEST_GUILESS_MAIN(TestTreeBuilders)
#include "test_treebuilders.moc"
