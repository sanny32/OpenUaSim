// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QCheckBox>
#include <QClipboard>
#include <QFontMetricsF>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTableView>
#include <QTest>
#include <QToolButton>
#include <QToolTip>
#include <QtMath>
#include <QTreeView>

#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QStyleFactory>
#include <QTabBar>

#include "apppalette.h"
#include "appstyle.h"
#include "data/serverstatus.h"
#include "eventlogwidget.h"
#include "itemdelegate.h"
#include "itemroles.h"
#include "logmodel.h"
#include "mainstatusbarwidget.h"
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
    void mainStatusBar();
    void eventLogShowsAndClears();
    void appStylePanelsAndSeparators();
    void appStyleAccentButton();
    void appStyleSelectedTab();
    void appStyleEnlargesTabsAndToolbar();
    void appStyleSpacesToolbarButtonLabels();
    void eventLogUsesCompactRows();
    void eventLogHeightShowsWholeRows();
    void appStyleMakesPanelScrollAreasTransparent();
    void appStyleCheckBoxes();
    void appStyleShowsMnemonicsAfterAlt();
    void addCopyActionCopies();
    void configureTable();
    void fitColumnsWhenFilledFitsOnce();
    void fitColumnsWhenFilledCapsLongValues();
    void fitColumnsWhenFilledLeavesNoScrollBar();
    void splitByStretchFollowsStretchFactors();
    void applyInitialStateExpandsAndSelects();
    void applyInitialStateFallsBackToFirstRow();
    void selectById();
    void selectRow();
    void attachSearchFilters();
    void attachTableSearchFiltersAllColumns();
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
    QCOMPARE(option.decorationSize.width(), option.decorationSize.height());
    QVERIFY(option.decorationSize.height() < option.fontMetrics.height());
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
    ItemDelegate delegate;
    QStyleOptionViewItem option;
    QStyledItemDelegate plain;
    QVERIFY(delegate.sizeHint(option, model.index(0, 0)).height()
            > plain.sizeHint(option, model.index(0, 0)).height());
}

void TestWidgets::mainStatusBar()
{
    ServerStatus status;
    status.running = true;

    MainStatusBarWidget bar;
    bar.setServerStatus(status);
    bar.setTrailingText(QStringLiteral("Log entries: 27"));
    QCOMPARE(bar.trailingText(), QStringLiteral("Log entries: 27"));
    QCOMPARE(bar.findChild<StatusIndicator *>()->status(), StatusKind::Good);
    QCOMPARE(bar.findChild<StatusIndicator *>()->text(), QStringLiteral("Server running"));
    QVERIFY(!bar.findChild<QLabel *>(QStringLiteral("endpointLabel")));

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
    QCOMPARE(widget.title(), QStringLiteral("Event Log"));

    auto *view = widget.findChild<QTableView *>();
    QCOMPARE(view->model(), &model);
    QVERIFY(view->isColumnHidden(LogModel::ColCategory));

    widget.findChild<QPushButton *>(QStringLiteral("clearButton"))->click();
    QCOMPARE(model.rowCount(), 0);
}

void TestWidgets::appStylePanelsAndSeparators()
{
    AppStyle style;
    const QPalette palette = AppPalette::build(true);
    const auto centre = [](QWidget &widget) {
        const QImage image = widget.grab().toImage();
        return image.pixelColor(image.width() / 2, image.height() / 2);
    };

    QFrame panel;
    panel.setStyle(&style);
    panel.setPalette(palette);
    panel.setFrameShape(QFrame::StyledPanel);
    panel.setProperty(AppStyle::panelProperty, true);
    panel.resize(60, 40);
    QCOMPARE(centre(panel), palette.color(QPalette::Base));

    QFrame separator;
    separator.setStyle(&style);
    separator.setPalette(palette);
    separator.setFrameShape(QFrame::StyledPanel);
    separator.setProperty(AppStyle::separatorProperty, true);
    separator.resize(60, 1);
    QCOMPARE(separator.grab().toImage().pixelColor(30, 0), palette.color(QPalette::Mid));
}

void TestWidgets::appStyleAccentButton()
{
    AppStyle style;
    QPushButton plain(QStringLiteral("Stop"));
    QPushButton accent(QStringLiteral("Start"));
    for (QPushButton *button : { &plain, &accent }) {
        button->setStyle(&style);
        button->setPalette(AppPalette::build(true));
        button->resize(100, 30);
    }
    accent.setProperty(AppStyle::accentProperty, QStringLiteral("success"));

    const QColor plainFill = plain.grab().toImage().pixelColor(5, 15);
    const QColor accentFill = accent.grab().toImage().pixelColor(5, 15);
    QVERIFY(accentFill.green() > accentFill.red());
    QVERIFY(accentFill != plainFill);

    accent.setEnabled(false);
    plain.setEnabled(false);
    QCOMPARE(accent.grab().toImage().pixelColor(5, 15), plain.grab().toImage().pixelColor(5, 15));
}

void TestWidgets::appStyleSelectedTab()
{
    AppStyle style;
    const QPalette palette = AppPalette::build(false);
    QTabBar bar;
    bar.setStyle(&style);
    bar.setPalette(palette);
    bar.addTab(QStringLiteral("Status"));
    bar.addTab(QStringLiteral("Objects"));
    bar.setCurrentIndex(0);
    bar.resize(bar.sizeHint());

    const QImage image = bar.grab().toImage();
    const QRect selected = bar.tabRect(0);
    const QRect other = bar.tabRect(1);
    QCOMPARE(image.pixelColor(selected.left() + 3, selected.center().y()), palette.color(QPalette::Highlight));
    QVERIFY(image.pixelColor(other.left() + 3, other.center().y()) != palette.color(QPalette::Highlight));
}

void TestWidgets::appStyleEnlargesTabsAndToolbar()
{
    AppStyle style;
    QStyle *fusion = QStyleFactory::create(QStringLiteral("Fusion"));

    QStyleOptionTab tab;
    const QSize contents(50, 16);
    const QSize fusionTab = fusion->sizeFromContents(QStyle::CT_TabBarTab, &tab, contents);
    const QSize appTab = style.sizeFromContents(QStyle::CT_TabBarTab, &tab, contents);
    QVERIFY(appTab.width() > fusionTab.width());
    QVERIFY(appTab.height() > fusionTab.height());

    QPushButton plain;
    QPushButton toolbar;
    toolbar.setProperty(AppStyle::toolbarProperty, true);
    QStyleOptionButton button;
    const QSize plainSize = style.sizeFromContents(QStyle::CT_PushButton, &button, contents, &plain);
    const QSize toolbarSize = style.sizeFromContents(QStyle::CT_PushButton, &button, contents, &toolbar);
    QCOMPARE(plainSize, fusion->sizeFromContents(QStyle::CT_PushButton, &button, contents, &plain));
    QVERIFY(toolbarSize.height() > plainSize.height());
    QVERIFY(toolbarSize.width() > plainSize.width());
    delete fusion;
}

void TestWidgets::appStyleSpacesToolbarButtonLabels()
{
    AppStyle style;
    QPixmap square(22, 22);
    square.fill(Qt::red);

    for (bool flat : { true, false }) {
        QPushButton button(QIcon(square), QStringLiteral("Add Node"));
        button.setStyle(&style);
        button.setProperty(AppStyle::toolbarProperty, true);
        button.setFlat(flat);
        button.setIconSize(square.size());
        const int textWidth = button.fontMetrics().size(Qt::TextShowMnemonic, button.text()).width();
        if (flat)
            QCOMPARE(button.sizeHint().width(), 2 * 10 + 22 + 10 + textWidth);
        QVERIFY(button.sizeHint().height() >= 34);

        button.resize(button.sizeHint());
        const QImage image = button.grab().toImage();
        const int middle = image.height() / 2;
        int iconLeft = 0;
        while (iconLeft < image.width() && image.pixelColor(iconLeft, middle) != QColor(Qt::red))
            ++iconLeft;
        QCOMPARE(image.pixelColor(iconLeft + 21, middle), QColor(Qt::red));
        QVERIFY(image.pixelColor(iconLeft + 22, middle) != QColor(Qt::red));
        if (flat)
            QCOMPARE(iconLeft, 10);
        else
            QVERIFY(qAbs(iconLeft - (image.width() - (iconLeft + 22 + 10 + textWidth))) <= 1);

        // The gap repeats the background left of the icon; a glyph's own negative bearing (Liberation Sans 'A') may overhang it.
        const qreal bearing = QFontMetricsF(button.font()).leftBearing(button.text().front());
        const int gapEnd = iconLeft + 32 + qFloor(qMin(qreal(0), bearing));
        for (int x = iconLeft + 22; x < gapEnd; ++x) {
            for (int y = 0; y < image.height(); ++y)
                QVERIFY2(image.pixelColor(x, y) == image.pixelColor(iconLeft - 2, y), flat ? "flat" : "bordered");
        }
    }
}

void TestWidgets::eventLogUsesCompactRows()
{
    LogModel model;
    EventLogWidget widget;
    widget.setLogModel(&model);
    const auto *view = widget.findChild<QTableView *>();
    QTableView regular;
    ViewSetup::configureTable(&regular);
    QVERIFY(view->verticalHeader()->defaultSectionSize() < regular.verticalHeader()->defaultSectionSize());
}

void TestWidgets::eventLogHeightShowsWholeRows()
{
    LogModel model;
    EventLogWidget widget;
    widget.setLogModel(&model);
    widget.resize(800, 200);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    const auto *view = widget.findChild<QTableView *>();
    const int rowHeight = view->verticalHeader()->defaultSectionSize();
    const int chrome = widget.height() - view->viewport()->height();

    for (int height : { 200, 200 + rowHeight / 2, 300 }) {
        const int fitted = widget.heightForWholeRows(height);
        QVERIFY(fitted <= height);
        QVERIFY(fitted > height - rowHeight);
        QCOMPARE((fitted - chrome) % rowHeight, 0);
    }
    QCOMPARE(widget.heightForWholeRows(chrome), chrome + rowHeight);
}

void TestWidgets::appStyleShowsMnemonicsAfterAlt()
{
    AppStyle style;
    style.polish(qApp);
    QMenuBar bar;
    bar.addMenu(QStringLiteral("&File"));
    QCOMPARE(style.styleHint(QStyle::SH_UnderlineShortcut, nullptr, &bar), 0);

    QTest::keyPress(&bar, Qt::Key_Alt, Qt::AltModifier);
    QCOMPARE(style.styleHint(QStyle::SH_UnderlineShortcut, nullptr, &bar), 1);
    QTest::keyRelease(&bar, Qt::Key_Alt);
    QCOMPARE(style.styleHint(QStyle::SH_UnderlineShortcut, nullptr, &bar), 1);

    QTest::mousePress(&bar, Qt::LeftButton);
    QCOMPARE(style.styleHint(QStyle::SH_UnderlineShortcut, nullptr, &bar), 0);

    style.unpolish(qApp);
    QTest::keyPress(&bar, Qt::Key_Alt, Qt::AltModifier);
    QCOMPARE(style.styleHint(QStyle::SH_UnderlineShortcut, nullptr, &bar), 0);
}

void TestWidgets::appStyleMakesPanelScrollAreasTransparent()
{
    AppStyle style;
    QScrollArea plain;
    QScrollArea body;
    body.setProperty(AppStyle::panelBodyProperty, true);
    body.setWidget(new QWidget);
    body.widget()->setAutoFillBackground(true);
    style.polish(&plain);
    style.polish(&body);

    QVERIFY(plain.viewport()->autoFillBackground());
    QVERIFY(!body.viewport()->autoFillBackground());
    QVERIFY(!body.widget()->autoFillBackground());
}

void TestWidgets::appStyleCheckBoxes()
{
    AppStyle style;
    const QPalette palette = AppPalette::build(true);
    QCheckBox checked;
    QCheckBox unchecked;
    for (QCheckBox *box : { &checked, &unchecked }) {
        box->setStyle(&style);
        box->setPalette(palette);
        box->resize(box->sizeHint());
    }
    checked.setChecked(true);

    QStyleOptionButton option;
    option.initFrom(&checked);
    const QRect indicator = style.subElementRect(QStyle::SE_CheckBoxIndicator, &option, &checked);
    QCOMPARE(indicator.width(), style.pixelMetric(QStyle::PM_IndicatorWidth));

    const QPoint corner = indicator.topLeft()
                          + QPoint(indicator.width() * 4 / 5, indicator.height() * 4 / 5);
    QCOMPARE(checked.grab().toImage().pixelColor(corner), palette.color(QPalette::Highlight));
    QCOMPARE(unchecked.grab().toImage().pixelColor(indicator.center()), palette.color(QPalette::Base));
}

void TestWidgets::addCopyActionCopies()
{
    QLineEdit edit(QStringLiteral("opc.tcp://localhost:48400"));
    edit.setReadOnly(true);
    ViewSetup::addCopyAction(&edit);
    QCOMPARE(edit.actions().size(), 1);
    auto *button = edit.findChild<QToolButton *>();
    QVERIFY(button);
    QCOMPARE(button->cursor().shape(), Qt::PointingHandCursor);

    edit.show();
    QVERIFY(QTest::qWaitForWindowExposed(&edit));
    QTest::mouseClick(button, Qt::LeftButton);
    QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("opc.tcp://localhost:48400"));
    QTRY_VERIFY(QToolTip::isVisible());
    QCOMPARE(QToolTip::text(), QStringLiteral("Copied"));
    QToolTip::hideText();
}

void TestWidgets::configureTable()
{
    QStandardItemModel model(1, 3);
    QTableView view;
    view.setModel(&model);
    ViewSetup::configureTable(&view);
    QVERIFY(qobject_cast<ItemDelegate *>(view.itemDelegate()));
    const QHeaderView *header = view.horizontalHeader();
    QCOMPARE(header->defaultAlignment(), Qt::AlignLeft | Qt::AlignVCenter);
    for (int column = 0; column < model.columnCount(); ++column)
        QCOMPARE(header->sectionResizeMode(column), QHeaderView::Interactive);
    QVERIFY(header->stretchLastSection());
}

void TestWidgets::fitColumnsWhenFilledFitsOnce()
{
    QStandardItemModel model(0, 3);
    QTableView view;
    view.setModel(&model);
    ViewSetup::configureTable(&view);
    ViewSetup::fitColumnsWhenFilled(&view);
    QHeaderView *header = view.horizontalHeader();
    const int emptyWidth = header->sectionSize(1);

    model.appendRow({ new QStandardItem(QStringLiteral("0")),
                      new QStandardItem(QStringLiteral("http://opcfoundation.org/UA/DI/")),
                      new QStandardItem(QStringLiteral("1.05")) });
    QVERIFY(header->sectionSize(0) < header->sectionSize(1));
    QVERIFY(header->sectionSize(1) != emptyWidth);

    header->resizeSection(0, 150);
    model.appendRow({ new QStandardItem(QStringLiteral("1")),
                      new QStandardItem(QStringLiteral("urn:other")),
                      new QStandardItem(QStringLiteral("1.0.0")) });
    QCOMPARE(header->sectionSize(0), 150);
}

void TestWidgets::fitColumnsWhenFilledCapsLongValues()
{
    QStandardItemModel model(0, 2);
    model.appendRow({ new QStandardItem(QString(400, QLatin1Char('x'))), new QStandardItem(QStringLiteral("y")) });
    QTableView view;
    view.setModel(&model);
    ViewSetup::configureTable(&view);
    ViewSetup::fitColumnsWhenFilled(&view);
    QVERIFY(view.horizontalHeader()->sectionSize(0) <= 320);
    QVERIFY(view.horizontalHeader()->sectionSize(0) > 100);
}

void TestWidgets::fitColumnsWhenFilledLeavesNoScrollBar()
{
    QStandardItemModel model(0, 3);
    model.appendRow({ new QStandardItem(QString(40, QLatin1Char('x'))), new QStandardItem(QString(40, QLatin1Char('y'))),
                      new QStandardItem(QStringLiteral("3A:5F:7C:9D:12:34:56:78:9A:BC")) });
    QTableView view;
    view.setModel(&model);
    ViewSetup::configureTable(&view);
    view.verticalHeader()->hide();
    view.resize(900, 200);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    ViewSetup::fitColumnsWhenFilled(&view);
    const int fitted = view.horizontalHeader()->sectionSize(0) + view.horizontalHeader()->sectionSize(1);
    view.resize(fitted + 80, 200);
    QTRY_COMPARE(view.horizontalHeader()->length(), view.viewport()->width());
    QCOMPARE(view.horizontalScrollBar()->maximum(), 0);
}

void TestWidgets::splitByStretchFollowsStretchFactors()
{
    QSplitter splitter;
    for (int stretch : { 1, 2, 0 }) {
        auto *pane = new QWidget;
        QSizePolicy policy = pane->sizePolicy();
        policy.setHorizontalStretch(stretch);
        pane->setSizePolicy(policy);
        splitter.addWidget(pane);
    }
    splitter.resize(400, 100);
    splitter.show();
    QVERIFY(QTest::qWaitForWindowExposed(&splitter));

    ViewSetup::splitByStretch(&splitter);
    const QList<int> sizes = splitter.sizes();
    QCOMPARE(sizes.size(), 3);
    const QByteArray text = QStringLiteral("%1 %2 %3").arg(sizes.at(0)).arg(sizes.at(1)).arg(sizes.at(2)).toUtf8();
    QVERIFY2(qAbs(sizes.at(1) - 2 * sizes.at(0)) <= 6, text.constData());
    QVERIFY2(qAbs(sizes.at(2) - sizes.at(0)) <= 6, text.constData());
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

void TestWidgets::attachTableSearchFiltersAllColumns()
{
    QStandardItemModel model(3, 2);
    model.setData(model.index(0, 0), QStringLiteral("localhost"));
    model.setData(model.index(1, 1), QStringLiteral("Basic256Sha256"));
    model.setData(model.index(2, 0), QStringLiteral("discovery"));
    QTableView view;
    QLineEdit search;
    QSortFilterProxyModel *proxy = ViewSetup::attachTableSearch(&view, &model, &search);

    QCOMPARE(view.model(), proxy);
    QCOMPARE(search.actions().size(), 1);
    search.setText(QStringLiteral("sha256"));
    QCOMPARE(proxy->rowCount(), 1);
    QCOMPARE(proxy->mapToSource(proxy->index(0, 0)).row(), 1);
    search.setText(QStringLiteral("DISCO"));
    QCOMPARE(proxy->rowCount(), 1);
    search.clear();
    QCOMPARE(proxy->rowCount(), 3);
}

void TestWidgets::bindCopyButtonCopies()
{
    QToolButton button;
    QLineEdit source(QStringLiteral("ns=2;s=Motor_01.Speed"));
    ViewSetup::bindCopyButton(&button, &source);
    QVERIFY(!button.icon().isNull());
    QCOMPARE(button.cursor().shape(), Qt::PointingHandCursor);

    button.show();
    QVERIFY(QTest::qWaitForWindowExposed(&button));
    button.click();
    QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("ns=2;s=Motor_01.Speed"));
    QTRY_VERIFY(QToolTip::isVisible());
    QCOMPARE(QToolTip::text(), QStringLiteral("Copied"));
    QToolTip::hideText();
}

QTEST_MAIN(TestWidgets)
#include "test_widgets.moc"
