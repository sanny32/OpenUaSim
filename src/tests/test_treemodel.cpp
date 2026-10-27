// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QTest>

#include "itemroles.h"
#include "treemodel.h"

namespace {

TreeNode node(const QString &text, const QString &id, TreeNode::Kind kind = TreeNode::Kind::Folder)
{
    TreeNode result;
    result.text = text;
    result.id = id;
    result.kind = kind;
    return result;
}

QList<TreeNode> sampleTree()
{
    TreeNode root = node(QStringLiteral("Objects"), QStringLiteral("i=85"));
    root.icon = QStringLiteral("folder");
    root.expanded = true;
    TreeNode motor = node(QStringLiteral("Motor"), QStringLiteral("motor"), TreeNode::Kind::Object);
    TreeNode speed = node(QStringLiteral("Speed"), QStringLiteral("motor.speed"), TreeNode::Kind::Variable);
    speed.current = true;
    motor.children = { speed, node(QStringLiteral("Running"), QStringLiteral("motor.running")) };
    root.children = { motor };
    return { root, node(QStringLiteral("Views"), QStringLiteral("i=87")) };
}

}

class TestTreeModel : public QObject
{
    Q_OBJECT

private slots:
    void emptyModelHasNoRows();
    void buildsHierarchy();
    void passesModelTester();
    void exposesCustomRoles();
    void iconRoleIsInvalidWithoutIcon();
    void addsTrailingColumnOnlyWhenUsed();
    void findsNodesById();
    void setIconChangesOneNode();
    void setNodesResetsModel();
};

void TestTreeModel::emptyModelHasNoRows()
{
    TreeModel model;
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.columnCount(), 1);
    QVERIFY(!model.index(0, 0).isValid());
}

void TestTreeModel::buildsHierarchy()
{
    TreeModel model;
    model.setNodes(sampleTree());

    QCOMPARE(model.rowCount(), 2);
    const QModelIndex objects = model.index(0, 0);
    QCOMPARE(objects.data().toString(), QStringLiteral("Objects"));
    QCOMPARE(model.rowCount(objects), 1);

    const QModelIndex motor = model.index(0, 0, objects);
    QCOMPARE(model.rowCount(motor), 2);
    const QModelIndex running = model.index(1, 0, motor);
    QCOMPARE(running.data().toString(), QStringLiteral("Running"));
    QCOMPARE(model.parent(running), motor);
    QCOMPARE(model.parent(motor), objects);
    QVERIFY(!model.parent(objects).isValid());
    QVERIFY(!model.index(5, 0, motor).isValid());
}

void TestTreeModel::passesModelTester()
{
    TreeModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    model.setNodes(sampleTree());
    TreeNode withTrailing = node(QStringLiteral("Trusted"), QStringLiteral("1"));
    withTrailing.trailing = QStringLiteral("3");
    model.setNodes({ withTrailing });
}

void TestTreeModel::exposesCustomRoles()
{
    TreeModel model;
    model.setNodes(sampleTree());
    const QModelIndex objects = model.index(0, 0);
    const QModelIndex speed = model.index(0, 0, model.index(0, 0, objects));

    QCOMPARE(objects.data(ItemRoles::IconName).toString(), QStringLiteral("folder"));
    QCOMPARE(objects.data(ItemRoles::Expanded).toBool(), true);
    QCOMPARE(objects.data(ItemRoles::Current).toBool(), false);
    QCOMPARE(speed.data(ItemRoles::Id).toString(), QStringLiteral("motor.speed"));
    QCOMPARE(speed.data(ItemRoles::NodeKind).toInt(), static_cast<int>(TreeNode::Kind::Variable));
    QCOMPARE(speed.data(ItemRoles::Current).toBool(), true);
    QVERIFY(!speed.data(Qt::ToolTipRole).isValid());
    QVERIFY(!objects.data(ItemRoles::SeparatorAbove).toBool());

    TreeNode apart = node(QStringLiteral("All"), QStringLiteral("all"));
    apart.trailing = QStringLiteral("2");
    QVERIFY(!objects.data(ItemRoles::Muted).toBool());

    apart.separatorAbove = true;
    apart.muted = true;
    model.setNodes({ apart });
    QVERIFY(model.index(0, 0).data(ItemRoles::Muted).toBool());
    QVERIFY(model.index(0, 0).data(ItemRoles::SeparatorAbove).toBool());
    QVERIFY(model.index(0, 1).data(ItemRoles::SeparatorAbove).toBool());
}

void TestTreeModel::iconRoleIsInvalidWithoutIcon()
{
    TreeModel model;
    model.setNodes(sampleTree());
    QVERIFY(!model.index(1, 0).data(ItemRoles::IconName).isValid());
}

void TestTreeModel::addsTrailingColumnOnlyWhenUsed()
{
    TreeModel model;
    model.setNodes(sampleTree());
    QCOMPARE(model.columnCount(), 1);

    TreeNode category = node(QStringLiteral("Trusted"), QStringLiteral("1"));
    category.trailing = QStringLiteral("3");
    model.setNodes({ category });
    QCOMPARE(model.columnCount(), 2);
    const QModelIndex count = model.index(0, 1);
    QCOMPARE(count.data().toString(), QStringLiteral("3"));
    QCOMPARE(count.data(Qt::TextAlignmentRole).value<Qt::Alignment>(), Qt::AlignRight | Qt::AlignVCenter);
    QVERIFY(!count.data(ItemRoles::Id).isValid());
    QCOMPARE(model.rowCount(count), 0);
}

void TestTreeModel::findsNodesById()
{
    TreeModel model;
    model.setNodes(sampleTree());

    const QModelIndex running = model.findById(QStringLiteral("motor.running"));
    QVERIFY(running.isValid());
    QCOMPARE(running.data().toString(), QStringLiteral("Running"));
    QVERIFY(model.findById(QStringLiteral("i=87")).isValid());
    QVERIFY(!model.findById(QStringLiteral("missing")).isValid());
    QVERIFY(!model.findById(QString()).isValid());
}

void TestTreeModel::setIconChangesOneNode()
{
    TreeModel model;
    model.setNodes(sampleTree());
    QSignalSpy changed(&model, &TreeModel::dataChanged);

    QVERIFY(model.setIcon(QStringLiteral("motor.running"), QStringLiteral("zap")));
    const QModelIndex running = model.findById(QStringLiteral("motor.running"));
    QCOMPARE(running.data(ItemRoles::IconName).toString(), QStringLiteral("zap"));
    QCOMPARE(changed.count(), 1);
    QCOMPARE(changed.first().at(0).toModelIndex(), running);

    QVERIFY(!model.setIcon(QStringLiteral("missing"), QStringLiteral("zap")));
    QCOMPARE(changed.count(), 1);
    QVERIFY(model.setIcon(QStringLiteral("motor.speed"), QStringLiteral("zap")));
    QVERIFY(model.setIcon(QStringLiteral("motor.speed"), QString()));
    QCOMPARE(model.findById(QStringLiteral("motor.speed")).data(ItemRoles::IconName).toString(),
             QStringLiteral("variable-type"));
}

void TestTreeModel::setNodesResetsModel()
{
    TreeModel model;
    QSignalSpy reset(&model, &TreeModel::modelReset);
    model.setNodes(sampleTree());
    model.setNodes({});
    QCOMPARE(reset.count(), 2);
    QCOMPARE(model.rowCount(), 0);
}

QTEST_GUILESS_MAIN(TestTreeModel)
#include "test_treemodel.moc"
