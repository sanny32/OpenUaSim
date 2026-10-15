// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QClipboard>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTableView>
#include <QTest>
#include <QToolButton>
#include <QTreeView>

#include "data/serverstatus.h"
#include "eventlogwidget.h"
#include "itemdelegate.h"
#include "itemroles.h"
#include "logmodel.h"
#include "mainstatusbarwidget.h"
#include "statcardwidget.h"
#include "statusindicator.h"
#include "treemodel.h"
#include "viewsetup.h"

namespace {

class InspectableDelegate : public ItemDelegate
{
public:
    QStyleOptionViewItem optionFor(const QModelIndex &index) const
    {
        QStyleOptionViewItem option;
        initStyleOption(&option, index);
        return option;
    }
};

QList<TreeNode> sampleTree()
{
    TreeNode leaf;
    leaf.text = QStringLiteral("Speed");
    leaf.id = QStringLiteral("speed");
    leaf.current = true;
    TreeNode other;
    other.text = QStringLiteral("Running");
    other.id = QStringLiteral("running");
    TreeNode motor;
    motor.text = QStringLiteral("Motor");
    motor.id = QStringLiteral("motor");
    motor.expanded = true;
    motor.children = { leaf, other };
    TreeNode views;
    views.text = QStringLiteral("Views");
    views.id = QStringLiteral("views");
    return { motor, views };
}

}

class TestWidgets : public QObject
{
    Q_OBJECT

private slots:
    void statusIndicator();
    void delegateDecoratesIconNames();
    void delegateDecoratesStatus();
    void delegateLeavesPlainCellsAlone();
    void delegateRaisesRowHeight();
    void statCard();
    void mainStatusBar();
    void eventLogShowsAndClears();
    void configureTable();
    void applyInitialStateExpandsAndSelects();
    void applyInitialStateFallsBackToFirstRow();
    void selectById();
    void selectRow();
    void attachSearchFilters();
    void bindCopyButtonCopies();
};

void TestWidgets::statusIndicator()
{
    StatusIndicator indicator;
    const int emptyWidth = indicator.sizeHint().width();
    indicator.setStatus(StatusKind::Good, QStringLiteral("Running"));
    QCOMPARE(indicator.status(), StatusKind::Good);
    QCOMPARE(indicator.text(), QStringLiteral("Running"));
    QVERIFY(indicator.sizeHint().width() > emptyWidth);
    QCOMPARE(indicator.minimumSizeHint(), indicator.sizeHint());
    indicator.setColoredText(true);
    indicator.resize(indicator.sizeHint());
    QVERIFY(!indicator.grab().isNull());
}

void TestWidgets::delegateDecoratesIconNames()
{
    QStandardItemModel model(1, 1);
    model.setData(model.index(0, 0), QStringLiteral("folder"), ItemRoles::IconName);
    InspectableDelegate delegate;
    const QStyleOptionViewItem option = delegate.optionFor(model.index(0, 0));
    QVERIFY(option.features.testFlag(QStyleOptionViewItem::HasDecoration));
    QVERIFY(!option.icon.isNull());
}

void TestWidgets::delegateDecoratesStatus()
{
    QStandardItemModel model(1, 1);
    model.setData(model.index(0, 0), ItemRoles::statusValue(StatusKind::Warning), ItemRoles::Status);
    InspectableDelegate delegate;
    const QStyleOptionViewItem option = delegate.optionFor(model.index(0, 0));
    QVERIFY(option.features.testFlag(QStyleOptionViewItem::HasDecoration));
    QCOMPARE(option.decorationSize, QSize(16, 16));
}

void TestWidgets::delegateLeavesPlainCellsAlone()
{
    QStandardItemModel model(1, 1);
    model.setData(model.index(0, 0), QStringLiteral("text"));
    InspectableDelegate delegate;
    const QStyleOptionViewItem option = delegate.optionFor(model.index(0, 0));
    QVERIFY(!option.features.testFlag(QStyleOptionViewItem::HasDecoration));
    QVERIFY(option.icon.isNull());
}

void TestWidgets::delegateRaisesRowHeight()
{
    QStandardItemModel model(1, 1);
    model.setData(model.index(0, 0), QStringLiteral("text"));
    model.insertRow(1);
    model.setData(model.index(1, 0), QStringLiteral("name\nopc.tcp://localhost:4840"));
    ItemDelegate delegate;
    QStyleOptionViewItem option;
    const int single = delegate.sizeHint(option, model.index(0, 0)).height();
    QVERIFY(single >= 30);
    QVERIFY(delegate.sizeHint(option, model.index(1, 0)).height() > single);
}

void TestWidgets::statCard()
{
    StatCardWidget card;
    card.setIcon(QStringLiteral("users"), Qt::blue);
    card.setCaption(QStringLiteral("Clients Connected"));
    card.setValue(QStringLiteral("1"));
    QCOMPARE(card.value(), QStringLiteral("1"));
    QVERIFY(!card.findChild<QLabel *>(QStringLiteral("iconLabel"))->pixmap().isNull());

    card.setChevronVisible(false);
    QVERIFY(card.findChild<QLabel *>(QStringLiteral("chevronLabel"))->isHidden());
}

void TestWidgets::mainStatusBar()
{
    ServerStatus status;
    status.running = true;
    status.primaryEndpoint = QStringLiteral("opc.tcp://localhost:4840");
    status.clientsConnected = 1;

    MainStatusBarWidget bar;
    bar.setServerStatus(status);
    bar.setTrailingText(QStringLiteral("Nodes: 27"));
    QCOMPARE(bar.trailingText(), QStringLiteral("Nodes: 27"));
    QCOMPARE(bar.findChild<StatusIndicator *>()->status(), StatusKind::Good);
    QCOMPARE(bar.findChild<QLabel *>(QStringLiteral("endpointLabel"))->text(),
             QStringLiteral("Endpoint: opc.tcp://localhost:4840"));

    status.running = false;
    bar.setServerStatus(status);
    QCOMPARE(bar.findChild<StatusIndicator *>()->status(), StatusKind::Inactive);
}

void TestWidgets::eventLogShowsAndClears()
{
    LogModel model;
    LogEntry entry;
    entry.message = QStringLiteral("started");
    model.setItems({ entry, entry });

    EventLogWidget widget;
    widget.setLogModel(&model);
    widget.setTitle(QStringLiteral("Recent Activity"));
    QCOMPARE(widget.title(), QStringLiteral("Recent Activity"));

    auto *view = widget.findChild<QTableView *>();
    QCOMPARE(view->model(), &model);
    QVERIFY(view->isColumnHidden(LogModel::ColCategory));

    widget.findChild<QPushButton *>(QStringLiteral("clearButton"))->click();
    QCOMPARE(model.rowCount(), 0);
}

void TestWidgets::configureTable()
{
    QStandardItemModel model(1, 3);
    QTableView stretched;
    stretched.setModel(&model);
    ViewSetup::configureTable(&stretched);
    QVERIFY(qobject_cast<ItemDelegate *>(stretched.itemDelegate()));
    QCOMPARE(stretched.horizontalHeader()->defaultAlignment(), Qt::AlignLeft | Qt::AlignVCenter);
    QCOMPARE(stretched.horizontalHeader()->sectionResizeMode(1), QHeaderView::Stretch);

    QTableView interactive;
    interactive.setModel(&model);
    ViewSetup::configureTable(&interactive, ViewSetup::ColumnSizing::Interactive);
    QCOMPARE(interactive.horizontalHeader()->sectionResizeMode(1), QHeaderView::Interactive);
}

void TestWidgets::applyInitialStateExpandsAndSelects()
{
    TreeModel model;
    model.setNodes(sampleTree());
    QTreeView view;
    view.setModel(&model);

    ViewSetup::applyInitialState(&view);
    QVERIFY(view.isExpanded(model.index(0, 0)));
    QVERIFY(!view.isExpanded(model.index(1, 0)));
    QCOMPARE(ViewSetup::currentId(&view), QStringLiteral("speed"));
}

void TestWidgets::applyInitialStateFallsBackToFirstRow()
{
    QList<TreeNode> tree = sampleTree();
    tree[0].children[0].current = false;
    TreeModel model;
    model.setNodes(tree);
    QTreeView view;
    view.setModel(&model);

    ViewSetup::applyInitialState(&view);
    QCOMPARE(ViewSetup::currentId(&view), QStringLiteral("motor"));
}

void TestWidgets::selectById()
{
    TreeModel model;
    model.setNodes(sampleTree());
    QTreeView view;
    view.setModel(&model);

    QVERIFY(ViewSetup::selectById(&view, QStringLiteral("running")));
    QCOMPARE(ViewSetup::currentId(&view), QStringLiteral("running"));
    QVERIFY(!ViewSetup::selectById(&view, QStringLiteral("missing")));
    QCOMPARE(ViewSetup::currentId(&view), QStringLiteral("running"));
}

void TestWidgets::selectRow()
{
    QStandardItemModel model(3, 2);
    QTableView view;
    view.setModel(&model);

    ViewSetup::selectRow(&view, 2);
    QCOMPARE(view.currentIndex().row(), 2);
    ViewSetup::selectRow(&view, 7);
    QVERIFY(!view.currentIndex().isValid());
    QVERIFY(!view.selectionModel()->hasSelection());
    QCOMPARE(ViewSetup::currentId(&view), QString());
}

void TestWidgets::attachSearchFilters()
{
    TreeModel model;
    model.setNodes(sampleTree());
    QTreeView view;
    QLineEdit search;
    QSortFilterProxyModel *proxy = ViewSetup::attachSearch(&view, &model, &search);

    QCOMPARE(view.model(), proxy);
    QCOMPARE(search.actions().size(), 1);
    search.setText(QStringLiteral("runn"));
    QCOMPARE(proxy->rowCount(), 1);
    const QModelIndex motor = proxy->index(0, 0);
    QCOMPARE(proxy->rowCount(motor), 1);
    QCOMPARE(proxy->index(0, 0, motor).data().toString(), QStringLiteral("Running"));

    search.clear();
    QCOMPARE(proxy->rowCount(), 2);
}

void TestWidgets::bindCopyButtonCopies()
{
    QToolButton button;
    QLineEdit source(QStringLiteral("ns=2;s=Motor_01.Speed"));
    ViewSetup::bindCopyButton(&button, &source);
    QVERIFY(!button.icon().isNull());

    button.click();
    QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("ns=2;s=Motor_01.Speed"));
}

QTEST_MAIN(TestWidgets)
#include "test_widgets.moc"
