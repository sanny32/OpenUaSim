// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QTest>

#include "treebuilders.h"

namespace {

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
    void certificateCategories();
    void certificateCategoriesWithoutCertificates();
};

void TestTreeBuilders::certificateCategories()
{
    using Category = CertificateInfo::Category;
    const QList<TreeNode> nodes = TreeBuilders::certificateCategories({
        certificate(Category::Trusted), certificate(Category::Trusted),
        certificate(Category::Rejected), certificate(Category::Trusted),
    });

    QCOMPARE(nodes.size(), 5);
    QCOMPARE(nodes.at(0).text, QStringLiteral("All Certificates"));
    QCOMPARE(nodes.at(0).trailing, QStringLiteral("4"));
    QCOMPARE(nodes.at(0).id, TreeBuilders::allCertificatesId());
    QCOMPARE(nodes.at(1).text, QStringLiteral("Application Certificate"));
    QCOMPARE(nodes.at(1).trailing, QStringLiteral("0"));
    QCOMPARE(nodes.at(2).text, QStringLiteral("Trusted Clients"));
    QCOMPARE(nodes.at(2).trailing, QStringLiteral("3"));
    QCOMPARE(nodes.at(2).id, QString::number(static_cast<int>(Category::Trusted)));
    QCOMPARE(nodes.at(3).trailing, QStringLiteral("1"));
    QCOMPARE(nodes.at(4).text, QStringLiteral("Issuers"));
    for (qsizetype i = 1; i < nodes.size(); ++i) {
        bool isCategory = false;
        nodes.at(i).id.toInt(&isCategory);
        QVERIFY(isCategory);
    }
    bool allIsCategory = true;
    TreeBuilders::allCertificatesId().toInt(&allIsCategory);
    QVERIFY(!allIsCategory);
    for (const TreeNode &node : nodes) {
        QVERIFY(!node.icon.isEmpty());
        QCOMPARE(node.kind, TreeNode::Kind::Category);
    }
}

void TestTreeBuilders::certificateCategoriesWithoutCertificates()
{
    const QList<TreeNode> nodes = TreeBuilders::certificateCategories({});
    QCOMPARE(nodes.size(), 5);
    for (const TreeNode &node : nodes)
        QCOMPARE(node.trailing, QStringLiteral("0"));
}

QTEST_GUILESS_MAIN(TestTreeBuilders)
#include "test_treebuilders.moc"
