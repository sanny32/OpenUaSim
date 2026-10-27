// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QCheckBox>
#include <QHelpEvent>
#include <QClipboard>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTableView>
#include <QTest>
#include <QToolButton>
#include <QToolTip>
#include <QTreeView>

#include <QScrollArea>
#include <QScrollBar>
#include <QSignalSpy>
#include <QSpinBox>
#include <QSplitter>
#include <QStyleFactory>
#include <QTabBar>

#include "appicons.h"
#include "apppalette.h"
#include "appstyle.h"
#include "data/serverstatus.h"
#include "elidedlabel.h"
#include "elidedlineedit.h"
#include "elidedcombobox.h"
#include "eventlogwidget.h"
#include "formatting.h"
#include "itemdelegate.h"
#include "itemroles.h"
#include "logmodel.h"
#include "mainstatusbarwidget.h"
#include "propertynodewidget.h"
#include "statusindicator.h"
#include "treemodel.h"
#include "valuepreviewdelegate.h"
#include "variablepropertieswidget.h"
#include "variablesmodel.h"
#include "viewsetup.h"

namespace {

class InspectableDelegate : public ItemDelegate
{
public:
    QStyleOptionViewItem optionFor(const QModelIndex &index, QStyle::State state = QStyle::State_None) const
    {
        QStyleOptionViewItem option;
        option.state = state;
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
    void statusIndicatorFitsTextAtSizeHint_data();
    void statusIndicatorFitsTextAtSizeHint();
    void statusIndicatorShowsOverflowTooltip();
    void delegateDecoratesIconNames();
    void delegateDecoratesStatus();
    void delegateLeavesPlainCellsAlone();
    void delegateDropsFocusFrame();
    void delegateRaisesRowHeight();
    void delegateDrawsSeparatorAboveRow();
    void mainStatusBar();
    void eventLogShowsAndClears();
    void eventLogFollowsNewRowsAtBottom();
    void eventLogKeepsPositionWhenScrolledUp();
    void eventLogCloseButtonRequestsClose();
    void eventLogCopiesSelectedMessage();
    void appStyleMessageBoxIconSize();
    void appStyleMessageBoxVerticalSpace();
    void appStylePanelsAndSeparators();
    void appStyleAccentButton();
    void appStyleSelectedTab();
    void appStyleEnlargesTabsAndToolbar();
    void appStyleSpacesToolbarButtonLabels();
    void eventLogUsesCompactRows();
    void eventLogHeightShowsWholeRows();
    void appStyleMakesPanelScrollAreasTransparent();
    void appStyleCheckBoxes();
    void appStyleKeepsItemTextNearDecoration();
    void appStyleShowsMnemonicsAfterAlt();
    void addCopyActionCopies();
    void configureTable();
    void fitColumnsWhenFilledFitsOnce();
    void fitColumnsWhenFilledCapsLongValues();
    void fitColumnsWhenFilledScrollsToLastColumn();
    void elidedCellShowsFullTextTooltip();
    void valuePreviewOffersFullValue();
    void elidedLineEditElidesUnfocusedText();
    void elidedLineEditShowsShortTextPlainly();
    void elidedLineEditKeepsTypingOrder();
    void elidedLineEditReservesActionSpace_data();
    void elidedLineEditReservesActionSpace();
    void elidedComboBoxShowsOverflowTooltip();
    void elidedComboBoxKeepsManualInput();
    void elidedControlsKeepExplicitTooltips();
    void elidedLabelKeepsFullValue();
    void elidedControlsMaskPasswordTooltips();
    void growColumnWithContentsNeverShrinks();
    void growColumnWithContentsKeepsWiderUserWidth();
    void fillVariableIconComboListsResolvableIcons();
    void splitByStretchFollowsStretchFactors();
    void splitByStretchKeepsSidePaneWidths();
    void splitByStretchResizesFirstOfTwoPanes();
    void applyInitialStateExpandsAndSelects();
    void applyInitialStateFallsBackToFirstRow();
    void selectById();
    void selectRow();
    void attachSearchFilters();
    void attachTableSearchFiltersAllColumns();
    void bindCopyButtonCopies();
    void variablePropertiesEditsVariable();
    void variablePropertiesShowsCurrentQuality_data();
    void variablePropertiesShowsCurrentQuality();
    void variablePropertiesLocksReadOnlyAndModelVariables();
    void propertyNodeEditsValue();
    void propertyNodeEditsTextValue();
    void variablePropertiesEnablesApplyOnEdit_data();
    void variablePropertiesEnablesApplyOnEdit();
    void propertyNodeEnablesApplyOnEdit();
    void propertyNodeLocksReadOnlyProperty();
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

void TestWidgets::statusIndicatorFitsTextAtSizeHint_data()
{
    QTest::addColumn<QString>("text");
    QTest::newRow("good") << QStringLiteral("Good");
    QTest::newRow("bad") << QStringLiteral("Bad");
    QTest::newRow("uncertain") << QStringLiteral("Uncertain");
    QTest::newRow("running") << QStringLiteral("Server running");
}

void TestWidgets::statusIndicatorFitsTextAtSizeHint()
{
    QFETCH(QString, text);
    QFont font = QApplication::font();
    font.setPointSizeF(9.3);
    font.setHintingPreference(QFont::PreferNoHinting);
    for (qreal letterSpacing : { 0.0, 0.2, 0.4, 0.6, 0.8 }) {
        font.setLetterSpacing(QFont::AbsoluteSpacing, letterSpacing);
        StatusIndicator fitted;
        fitted.setFont(font);
        fitted.setStatus(StatusKind::Good, text);
        fitted.resize(fitted.sizeHint());
        StatusIndicator wide;
        wide.setFont(font);
        wide.setStatus(StatusKind::Good, text);
        wide.resize(fitted.sizeHint() + QSize(50, 0));
        const QImage expected = wide.grab().toImage().copy(QRect(QPoint(), fitted.size()));
        QCOMPARE(fitted.grab().toImage(), expected);
    }
}

void TestWidgets::statusIndicatorShowsOverflowTooltip()
{
    StatusIndicator indicator;
    const QString value = QStringLiteral("A long status message");
    indicator.setStatus(StatusKind::Good, value);
    indicator.setFixedSize(70, 30);
    indicator.show();
    QVERIFY(QTest::qWaitForWindowExposed(&indicator));
    const QPoint pos = indicator.rect().center();
    QHelpEvent hover(QEvent::ToolTip, pos, indicator.mapToGlobal(pos));
    QCoreApplication::sendEvent(&indicator, &hover);
    QTRY_VERIFY(QToolTip::isVisible());
    QVERIFY(QToolTip::text().contains(value.toHtmlEscaped()));
    QToolTip::hideText();
    indicator.setStatus(StatusKind::Good, QStringLiteral("OK"));
    QCoreApplication::sendEvent(&indicator, &hover);
    QTRY_VERIFY(!QToolTip::isVisible());
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

void TestWidgets::delegateDropsFocusFrame()
{
    QStandardItemModel model(1, 2);
    model.setData(model.index(0, 0), QStringLiteral("text"));
    model.setData(model.index(0, 1), QStringLiteral("signal"), ItemRoles::IconName);
    InspectableDelegate delegate;
    const QStyle::State focused = QStyle::State_HasFocus | QStyle::State_Selected | QStyle::State_Enabled;
    for (int column : { 0, 1 }) {
        const QStyleOptionViewItem option = delegate.optionFor(model.index(0, column), focused);
        QVERIFY(!option.state.testFlag(QStyle::State_HasFocus));
        QVERIFY(option.state.testFlag(QStyle::State_Selected));
    }
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

void TestWidgets::delegateDrawsSeparatorAboveRow()
{
    QStandardItemModel model(2, 1);
    model.setData(model.index(0, 0), QStringLiteral("Folder"));
    model.setData(model.index(1, 0), QStringLiteral("All"));
    model.setData(model.index(1, 0), true, ItemRoles::SeparatorAbove);
    ItemDelegate delegate;
    QStyleOptionViewItem option;
    option.palette = AppPalette::build(false);
    const int plainHeight = delegate.sizeHint(option, model.index(0, 0)).height();
    const int separatedHeight = delegate.sizeHint(option, model.index(1, 0)).height();
    QVERIFY(separatedHeight > plainHeight);

    QImage image(200, separatedHeight, QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    option.rect = image.rect();
    delegate.paint(&painter, option, model.index(1, 0));
    painter.end();
    QCOMPARE(image.pixelColor(100, (separatedHeight - plainHeight) / 2), option.palette.color(QPalette::Mid));
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

    auto *clear = widget.findChild<QToolButton *>(QStringLiteral("clearButton"));
    QVERIFY(!clear->icon().isNull());
    clear->click();
    QCOMPARE(model.rowCount(), 0);
}

void TestWidgets::eventLogFollowsNewRowsAtBottom()
{
    LogModel model;
    model.setItems(QList<LogEntry>(30));
    EventLogWidget widget;
    widget.setLogModel(&model);
    widget.resize(600, 200);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    const QScrollBar *bar = widget.findChild<QTableView *>()->verticalScrollBar();
    QVERIFY(bar->maximum() > 0);
    QCOMPARE(bar->value(), bar->maximum());

    model.appendItems(QList<LogEntry>(5));
    QApplication::processEvents();
    QCOMPARE(bar->value(), bar->maximum());
}

void TestWidgets::eventLogKeepsPositionWhenScrolledUp()
{
    LogModel model;
    model.setItems(QList<LogEntry>(30));
    EventLogWidget widget;
    widget.setLogModel(&model);
    widget.resize(600, 200);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QScrollBar *bar = widget.findChild<QTableView *>()->verticalScrollBar();
    bar->setValue(0);

    model.appendItems(QList<LogEntry>(5));
    QApplication::processEvents();
    QCOMPARE(bar->value(), 0);

    bar->setValue(bar->maximum());
    model.appendItems(QList<LogEntry>(5));
    QApplication::processEvents();
    QCOMPARE(bar->value(), bar->maximum());
}

void TestWidgets::eventLogCloseButtonRequestsClose()
{
    EventLogWidget widget;
    QSignalSpy closed(&widget, &EventLogWidget::closeRequested);
    auto *close = widget.findChild<QToolButton *>(QStringLiteral("closeButton"));
    QVERIFY(!close->icon().isNull());

    close->click();
    QCOMPARE(closed.count(), 1);
}

void TestWidgets::eventLogCopiesSelectedMessage()
{
    LogModel model;
    LogEntry first;
    first.message = QStringLiteral("Socket closed");
    LogEntry second;
    second.message = QStringLiteral("Cannot validate the server certificate");
    model.setItems({ first, second });
    EventLogWidget widget;
    widget.setLogModel(&model);
    auto *view = widget.findChild<QTableView *>();
    QCOMPARE(view->contextMenuPolicy(), Qt::ActionsContextMenu);
    QCOMPARE(view->actions().size(), 1);
    QAction *copy = view->actions().constFirst();
    QCOMPARE(copy->shortcut(), QKeySequence(QKeySequence::Copy));
    QVERIFY(!copy->icon().isNull());

    QGuiApplication::clipboard()->setText(QStringLiteral("untouched"));
    copy->trigger();
    QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("untouched"));

    view->selectRow(1);
    copy->trigger();
    QCOMPARE(QGuiApplication::clipboard()->text(), second.message);

    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    view->setFocus();
    view->selectRow(0);
    QTest::keySequence(view, QKeySequence::Copy);
    QCOMPARE(QGuiApplication::clipboard()->text(), first.message);
}

///
/// \brief Draws message box icons at the standard 32 logical pixels.
///
void TestWidgets::appStyleMessageBoxIconSize()
{
    AppStyle style;
    QMessageBox box;
    box.setStyle(&style);
    for (const auto icon : { QMessageBox::Warning, QMessageBox::Information,
                             QMessageBox::Critical, QMessageBox::Question }) {
        box.setIcon(icon);
        const QPixmap pixmap = box.iconPixmap();
        QVERIFY(!pixmap.isNull());
        QCOMPARE(pixmap.deviceIndependentSize(), QSizeF(32, 32));
    }
    box.setIcon(QMessageBox::NoIcon);
    QVERIFY(box.iconPixmap().isNull());
}

///
/// \brief Preserves breathing room around short and wrapped message box text.
///
void TestWidgets::appStyleMessageBoxVerticalSpace()
{
    AppStyle style;
    for (const QString &text : { QStringLiteral("Save changes?"),
                                QStringLiteral("Save changes?\nThe project has unsaved settings.") }) {
        QMessageBox box(QMessageBox::Warning, QStringLiteral("Unsaved Changes"), text,
                        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        box.setStyle(&style);
        box.show();
        const auto *layout = qobject_cast<QGridLayout *>(box.layout());
        QVERIFY(layout);
        QVERIFY(layout->contentsMargins().top() >= 20);
        QVERIFY(layout->contentsMargins().bottom() >= 20);
        QVERIFY(layout->verticalSpacing() >= 14);
        const auto *buttons = box.findChild<QDialogButtonBox *>();
        QVERIFY(buttons);
        QVERIFY(box.height() - buttons->geometry().bottom() >= 20);
        QCOMPARE(box.iconPixmap().deviceIndependentSize(), QSizeF(32, 32));
    }
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
    const QSize fusionButton = fusion->sizeFromContents(QStyle::CT_PushButton, &button, contents, &plain);
    QCOMPARE(plainSize.height(), qMax(fusionButton.height(), 30));
    QVERIFY(plainSize.width() > fusionButton.width());
    QLineEdit edit;
    QStyleOptionFrame frame;
    QCOMPARE(plainSize.height(), style.sizeFromContents(QStyle::CT_LineEdit, &frame, contents, &edit).height());
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
        // FreeType's LCD filter bleeds a glyph's edge into the neighbouring pixel's blue channel.
        QFont font = button.font();
        font.setStyleStrategy(QFont::NoSubpixelAntialias);
        button.setFont(font);
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

        // The gap repeats the background left of the icon, so no text intrudes into it.
        for (int x = iconLeft + 22; x < iconLeft + 32; ++x) {
            for (int y = 0; y < image.height(); ++y) {
                const QColor background = image.pixelColor(iconLeft - 2, y);
                if (image.pixelColor(x, y) != background)
                    QFAIL(qPrintable(QStringLiteral("%1: pixel (%2, %3) is %4, background %5; icon at %6, font %7 %8px")
                                         .arg(flat ? QStringLiteral("flat") : QStringLiteral("bordered"))
                                         .arg(x).arg(y)
                                         .arg(image.pixelColor(x, y).name(QColor::HexArgb), background.name(QColor::HexArgb))
                                         .arg(iconLeft)
                                         .arg(QFontInfo(button.font()).family())
                                         .arg(QFontInfo(button.font()).pixelSize())));
            }
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

void TestWidgets::appStyleKeepsItemTextNearDecoration()
{
    AppStyle style;
    QTableView view;
    QStyleOptionViewItem option;
    option.initFrom(&view);
    option.rect = QRect(0, 0, 200, 27);
    option.text = QStringLiteral("Yes");
    option.features = QStyleOptionViewItem::HasDisplay;
    const QRect plainText = style.subElementRect(QStyle::SE_ItemViewItemText, &option, &view);
    const std::unique_ptr<QStyle> fusion(QStyleFactory::create(QStringLiteral("Fusion")));
    QCOMPARE(plainText, fusion->subElementRect(QStyle::SE_ItemViewItemText, &option, &view));

    option.features |= QStyleOptionViewItem::HasDecoration;
    option.decorationSize = QSize(12, 12);
    option.icon = AppIcons::statusDot(StatusKind::Good);
    const QRect decoration = style.subElementRect(QStyle::SE_ItemViewItemDecoration, &option, &view);
    const QRect text = style.subElementRect(QStyle::SE_ItemViewItemText, &option, &view);
    const int textPadding = style.pixelMetric(QStyle::PM_FocusFrameHMargin, &option, &view) + 1;
    QCOMPARE(text.left() + textPadding - (decoration.right() + 1), 6);
    QCOMPARE(text.right(), plainText.right());

    option.direction = Qt::RightToLeft;
    const QRect rtlDecoration = style.subElementRect(QStyle::SE_ItemViewItemDecoration, &option, &view);
    const QRect rtlText = style.subElementRect(QStyle::SE_ItemViewItemText, &option, &view);
    QCOMPARE(rtlDecoration.left() - (rtlText.right() + 1 - textPadding), 6);
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

void TestWidgets::fitColumnsWhenFilledScrollsToLastColumn()
{
    QStandardItemModel model(0, 2);
    model.appendRow({ new QStandardItem(QStringLiteral("x")), new QStandardItem(QString(120, QLatin1Char('y'))) });
    QTableView view;
    view.setModel(&model);
    ViewSetup::configureTable(&view);
    view.verticalHeader()->hide();
    view.resize(300, 200);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    ViewSetup::fitColumnsWhenFilled(&view);

    QVERIFY(view.horizontalHeader()->sectionSize(1) >= static_cast<const QAbstractItemView &>(view).sizeHintForColumn(1));
    QTRY_VERIFY(view.horizontalScrollBar()->maximum() > 0);

    view.resize(view.horizontalHeader()->length() + 200, 200);
    QTRY_COMPARE(view.horizontalHeader()->length(), view.viewport()->width());
    QCOMPARE(view.horizontalScrollBar()->maximum(), 0);
}

void TestWidgets::elidedCellShowsFullTextTooltip()
{
    const QString message = QStringLiteral("Could not add SecurityPolicy#EccNistP256 with error <BadInternalError>");
    QStandardItemModel model(0, 3);
    auto *own = new QStandardItem(message);
    own->setToolTip(QStringLiteral("Own tooltip"));
    model.appendRow({ new QStandardItem(message), new QStandardItem(QStringLiteral("x")), own });
    QTableView view;
    view.setModel(&model);
    ViewSetup::configureTable(&view);
    view.horizontalHeader()->setStretchLastSection(false);
    view.setColumnWidth(0, 60);
    view.setColumnWidth(1, 60);
    view.setColumnWidth(2, 60);
    view.resize(400, 200);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    const auto hover = [&view](int column) {
        const QPoint pos = view.visualRect(view.model()->index(0, column)).center();
        QHelpEvent event(QEvent::ToolTip, pos, view.viewport()->mapToGlobal(pos));
        QCoreApplication::sendEvent(view.viewport(), &event);
    };

    hover(0);
    QTRY_VERIFY(QToolTip::isVisible());
    QVERIFY(QToolTip::text().contains(message.toHtmlEscaped()));

    hover(2);
    QTRY_COMPARE(QToolTip::text(), QStringLiteral("Own tooltip"));

    hover(1);
    QTRY_VERIFY(!QToolTip::isVisible());
}

void TestWidgets::valuePreviewOffersFullValue()
{
    const QString full = QStringLiteral("first line\nsecond line with a < b");
    QStandardItemModel model(0, 1);
    auto *preview = new QStandardItem(QStringLiteral("first line"));
    preview->setData(full, Qt::UserRole);
    auto *complete = new QStandardItem(QStringLiteral("short"));
    complete->setData(QStringLiteral("short"), Qt::UserRole);
    model.appendRow(preview);
    model.appendRow(complete);
    model.appendRow(new QStandardItem(QStringLiteral("plain")));
    QTableView view;
    view.setModel(&model);
    auto *delegate = new ValuePreviewDelegate(&view);
    view.setItemDelegate(delegate);
    view.setColumnWidth(0, 300);
    view.resize(400, 200);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QSignalSpy expand(delegate, &ValuePreviewDelegate::expandRequested);
    const auto buttonPoint = [&view](int row) {
        const QRect cell = view.visualRect(view.model()->index(row, 0));
        return QPoint(cell.right() - 8, cell.center().y());
    };

    QTest::mouseClick(view.viewport(), Qt::LeftButton, {}, buttonPoint(1));
    QTest::mouseClick(view.viewport(), Qt::LeftButton, {}, buttonPoint(2));
    QCOMPARE(expand.count(), 0);
    QTest::mouseClick(view.viewport(), Qt::LeftButton, {}, buttonPoint(0));
    QCOMPARE(expand.count(), 1);
    QCOMPARE(expand.first().first().toModelIndex().row(), 0);

    const QPoint pos = view.visualRect(model.index(0, 0)).center();
    QHelpEvent event(QEvent::ToolTip, pos, view.viewport()->mapToGlobal(pos));
    QCoreApplication::sendEvent(view.viewport(), &event);
    QTRY_VERIFY(QToolTip::isVisible());
    QVERIFY(QToolTip::text().contains(QStringLiteral("white-space:pre-wrap")));
    QVERIFY(QToolTip::text().contains(QStringLiteral("a &lt; b")));
}

void TestWidgets::elidedLineEditElidesUnfocusedText()
{
    const QString id = QStringLiteral("ns=1;g=12dabdf1-380d-69ed-b34b-f40fc26c1a7e");
    QWidget window;
    auto *edit = new ElidedLineEdit(&window);
    auto *other = new QLineEdit(&window);
    edit->setReadOnly(true);
    edit->setGeometry(0, 0, 120, 24);
    other->setGeometry(0, 30, 120, 24);
    edit->setText(id);
    QCOMPARE(edit->cursorPosition(), 0);
    QVERIFY(edit->isElided());

    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    other->setFocus();
    const QImage unfocused = edit->grab().toImage();

    const QPoint pos = edit->rect().center();
    QHelpEvent hover(QEvent::ToolTip, pos, edit->mapToGlobal(pos));
    QCoreApplication::sendEvent(edit, &hover);
    QTRY_VERIFY(QToolTip::isVisible());
    QVERIFY(QToolTip::text().contains(id.toHtmlEscaped()));
    QToolTip::hideText();

    edit->setFocus();
    QTRY_VERIFY(edit->hasFocus());
    QVERIFY(edit->grab().toImage() != unfocused);
}

void TestWidgets::elidedLineEditShowsShortTextPlainly()
{
    ElidedLineEdit edit;
    edit.resize(300, 24);
    edit.setText(QStringLiteral("ns=1;i=1"));
    QVERIFY(!edit.isElided());
    QLineEdit plain;
    plain.resize(300, 24);
    plain.setText(edit.text());
    plain.setCursorPosition(0);
    QCOMPARE(edit.grab().toImage(), plain.grab().toImage());

    edit.show();
    QVERIFY(QTest::qWaitForWindowExposed(&edit));
    const QPoint pos = edit.rect().center();
    QHelpEvent hover(QEvent::ToolTip, pos, edit.mapToGlobal(pos));
    QCoreApplication::sendEvent(&edit, &hover);
    QTRY_VERIFY(!QToolTip::isVisible());
}

void TestWidgets::elidedLineEditKeepsTypingOrder()
{
    ElidedLineEdit edit;
    edit.setText(QStringLiteral("1000"));
    edit.selectAll();
    QTest::keyClicks(&edit, "1500");
    QCOMPARE(edit.text(), QStringLiteral("1500"));
    QCOMPARE(edit.cursorPosition(), 4);
}

void TestWidgets::elidedLineEditReservesActionSpace_data()
{
    QTest::addColumn<bool>("leading");
    QTest::newRow("copy") << false;
    QTest::newRow("search") << true;
}

void TestWidgets::elidedLineEditReservesActionSpace()
{
    QFETCH(bool, leading);
    const QString text = QStringLiteral("opc.tcp://machine.local:48400");
    ElidedLineEdit edit;
    edit.setReadOnly(true);
    edit.resize(edit.fontMetrics().horizontalAdvance(text) + 16, 34);
    edit.setText(text);
    edit.show();
    QVERIFY(QTest::qWaitForWindowExposed(&edit));
    QVERIFY(!edit.isElided());
    if (leading)
        edit.addAction(AppIcons::icon(QStringLiteral("search")), QLineEdit::LeadingPosition);
    else
        ViewSetup::addCopyAction(&edit);
    QCoreApplication::processEvents();
    QVERIFY(edit.isElided());
    QCOMPARE(edit.text(), text);

    const QPoint pos = edit.rect().center();
    QHelpEvent hover(QEvent::ToolTip, pos, edit.mapToGlobal(pos));
    QCoreApplication::sendEvent(&edit, &hover);
    QTRY_VERIFY(QToolTip::isVisible());
    QVERIFY(QToolTip::text().contains(text.toHtmlEscaped()));
    QToolTip::hideText();

    edit.resize(edit.width() + 200, edit.height());
    QVERIFY(!edit.isElided());
}

void TestWidgets::elidedComboBoxShowsOverflowTooltip()
{
    const QString value = QStringLiteral("A long server name with <plain> & unescaped characters");
    ElidedComboBox combo;
    combo.addItem(value, QStringLiteral("id"));
    combo.setFixedSize(130, 30);
    combo.show();
    QVERIFY(QTest::qWaitForWindowExposed(&combo));
    QVERIFY(combo.isElided());
    QCOMPARE(combo.currentText(), value);
    QCOMPARE(combo.currentData().toString(), QStringLiteral("id"));
    QVERIFY(qobject_cast<ItemDelegate *>(combo.view()->itemDelegate()));

    const QPoint pos = combo.rect().center();
    QHelpEvent hover(QEvent::ToolTip, pos, combo.mapToGlobal(pos));
    QCoreApplication::sendEvent(&combo, &hover);
    QTRY_VERIFY(QToolTip::isVisible());
    QVERIFY(QToolTip::text().contains(value.toHtmlEscaped()));
    QToolTip::hideText();

    combo.setItemText(0, QStringLiteral("short"));
    QVERIFY(!combo.isElided());
    QCoreApplication::sendEvent(&combo, &hover);
    QTRY_VERIFY(!QToolTip::isVisible());
}

void TestWidgets::elidedComboBoxKeepsManualInput()
{
    ElidedComboBox combo;
    combo.addItem(QStringLiteral("localhost"));
    combo.setEditable(true);
    auto *edit = qobject_cast<ElidedLineEdit *>(combo.lineEdit());
    QVERIFY(edit);
    QCOMPARE(combo.currentText(), QStringLiteral("localhost"));
    combo.setEditText(QStringLiteral("a-long-manually-entered-server-name.local"));
    combo.resize(130, 30);
    combo.show();
    QVERIFY(QTest::qWaitForWindowExposed(&combo));
    QVERIFY(combo.isElided());
    edit->setFocus();
    QTest::keyClick(edit, Qt::Key_End);
    QTest::keyClicks(edit, ":48400");
    QVERIFY(combo.currentText().endsWith(QStringLiteral(":48400")));
    combo.setEditable(false);
    QVERIFY(!combo.isEditable());
    combo.setEditable(true);
    QVERIFY(qobject_cast<ElidedLineEdit *>(combo.lineEdit()));
}

void TestWidgets::elidedLabelKeepsFullValue()
{
    ElidedLabel label;
    const QString value = QStringLiteral("opc.tcp://long-machine-name.local:48400 <plain> & text");
    label.setText(value);
    label.setTextInteractionFlags(Qt::TextSelectableByMouse);
    label.setFixedSize(130, 30);
    label.show();
    QVERIFY(QTest::qWaitForWindowExposed(&label));
    QVERIFY(label.isElided());
    QVERIFY(label.minimumSizeHint().width() < label.sizeHint().width());
    const QPoint pos = label.rect().center();
    QHelpEvent hover(QEvent::ToolTip, pos, label.mapToGlobal(pos));
    QCoreApplication::sendEvent(&label, &hover);
    QTRY_VERIFY(QToolTip::isVisible());
    QVERIFY(QToolTip::text().contains(value.toHtmlEscaped()));
    QToolTip::hideText();
    label.setSelection(0, value.size());
    QCOMPARE(label.selectedText(), value);
    label.setSelection(0, 0);
    label.setText(QStringLiteral("short"));
    QVERIFY(!label.isElided());
    QCoreApplication::sendEvent(&label, &hover);
    QTRY_VERIFY(!QToolTip::isVisible());
    label.setText(value);
    label.setWordWrap(true);
    QVERIFY(!label.isElided());
}

void TestWidgets::elidedControlsMaskPasswordTooltips()
{
    ElidedComboBox combo;
    combo.setEditable(true);
    combo.lineEdit()->setEchoMode(QLineEdit::Password);
    combo.setEditText(QString(200, QLatin1Char('x')));
    combo.setFixedSize(100, 30);
    combo.show();
    QVERIFY(QTest::qWaitForWindowExposed(&combo));
    QVERIFY(combo.isElided());
    for (QWidget *widget : { static_cast<QWidget *>(&combo), static_cast<QWidget *>(combo.lineEdit()) }) {
        const QPoint pos = widget->rect().center();
        QHelpEvent hover(QEvent::ToolTip, pos, widget->mapToGlobal(pos));
        QCoreApplication::sendEvent(widget, &hover);
        QTRY_VERIFY(QToolTip::isVisible());
        QVERIFY(QToolTip::text().contains(combo.lineEdit()->displayText().toHtmlEscaped()));
        QVERIFY(!QToolTip::text().contains(combo.currentText()));
        QToolTip::hideText();
    }
}

void TestWidgets::elidedControlsKeepExplicitTooltips()
{
    ElidedLineEdit edit;
    ElidedComboBox combo;
    ElidedLabel label;
    edit.setText(QString(200, QLatin1Char('x')));
    combo.addItem(edit.text());
    label.setText(edit.text());
    for (QWidget *widget : { static_cast<QWidget *>(&edit), static_cast<QWidget *>(&combo),
                             static_cast<QWidget *>(&label) }) {
        widget->setFixedSize(100, 30);
        widget->setToolTip(QStringLiteral("Own tooltip"));
        widget->show();
        QVERIFY(QTest::qWaitForWindowExposed(widget));
        const QPoint pos = widget->rect().center();
        QHelpEvent hover(QEvent::ToolTip, pos, widget->mapToGlobal(pos));
        QCoreApplication::sendEvent(widget, &hover);
        QTRY_COMPARE(QToolTip::text(), QStringLiteral("Own tooltip"));
        QToolTip::hideText();
    }
}

void TestWidgets::growColumnWithContentsNeverShrinks()
{
    QStandardItemModel model(0, 3);
    model.appendRow({ new QStandardItem(QStringLiteral("Speed")), new QStandardItem(QStringLiteral("1")),
                      new QStandardItem(QStringLiteral("Good")) });
    QTableView view;
    view.setModel(&model);
    ViewSetup::configureTable(&view);
    ViewSetup::fitColumnsWhenFilled(&view);
    ViewSetup::growColumnWithContents(&view, 1);
    QHeaderView *header = view.horizontalHeader();
    const int fitted = header->sectionSize(1);

    model.setData(model.index(0, 1), QStringLiteral("1,234,567.891234"));
    const int grown = header->sectionSize(1);
    QVERIFY(grown > fitted);
    QVERIFY(grown >= static_cast<const QAbstractItemView &>(view).sizeHintForColumn(1));

    model.setData(model.index(0, 1), QStringLiteral("2"));
    QCOMPARE(header->sectionSize(1), grown);

    const int nameWidth = header->sectionSize(0);
    model.setData(model.index(0, 0), QString(60, QLatin1Char('x')));
    QCOMPARE(header->sectionSize(0), nameWidth);
}

void TestWidgets::growColumnWithContentsKeepsWiderUserWidth()
{
    QStandardItemModel model(1, 2);
    QTableView view;
    view.setModel(&model);
    ViewSetup::configureTable(&view);
    ViewSetup::growColumnWithContents(&view, 0);
    view.horizontalHeader()->resizeSection(0, 250);
    model.setData(model.index(0, 0), QStringLiteral("42"));
    QCOMPARE(view.horizontalHeader()->sectionSize(0), 250);

    model.setData(model.index(0, 0), QString(400, QLatin1Char('x')));
    QCOMPARE(view.horizontalHeader()->sectionSize(0), 320);
}

void TestWidgets::fillVariableIconComboListsResolvableIcons()
{
    QComboBox combo;
    ViewSetup::fillVariableIconCombo(&combo);
    QCOMPARE(combo.count(), AppIcons::variableIcons().size());
    for (const QString &name : { QStringLiteral("signal"), QStringLiteral("image"),
                                 QStringLiteral("chart"), QStringLiteral("network"),
                                 QStringLiteral("database") })
        QVERIFY2(combo.findData(name) >= 0, qPrintable(name));
    for (int index = 0; index < combo.count(); ++index) {
        QVERIFY2(!combo.itemIcon(index).isNull(), qPrintable(combo.itemData(index).toString()));
        QVERIFY(!combo.itemText(index).isEmpty());
    }
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

void TestWidgets::splitByStretchKeepsSidePaneWidths()
{
    QSplitter splitter;
    for (int stretch : { 1, 2, 1 }) {
        auto *pane = new QWidget;
        QSizePolicy policy = pane->sizePolicy();
        policy.setHorizontalStretch(stretch);
        pane->setSizePolicy(policy);
        splitter.addWidget(pane);
    }
    splitter.resize(400, 100);
    ViewSetup::splitByStretch(&splitter);
    splitter.show();
    QVERIFY(QTest::qWaitForWindowExposed(&splitter));

    const QList<int> initial = splitter.sizes();
    const QByteArray initialText =
        QStringLiteral("%1 %2 %3").arg(initial.at(0)).arg(initial.at(1)).arg(initial.at(2)).toUtf8();
    QVERIFY2(qAbs(initial.at(1) - 2 * initial.at(0)) <= 6, initialText.constData());
    QVERIFY2(qAbs(initial.at(2) - initial.at(0)) <= 6, initialText.constData());

    splitter.resize(800, 100);
    QTRY_COMPARE(splitter.sizes().at(2), initial.at(2));
    QCOMPARE(splitter.sizes().at(0), initial.at(0));
    QVERIFY(splitter.sizes().at(1) > initial.at(1));

    splitter.resize(600, 100);
    QTRY_COMPARE(splitter.sizes().at(2), initial.at(2));
    QCOMPARE(splitter.sizes().at(0), initial.at(0));
}

void TestWidgets::splitByStretchResizesFirstOfTwoPanes()
{
    QSplitter splitter;
    for (int stretch : { 3, 1 }) {
        auto *pane = new QWidget;
        QSizePolicy policy = pane->sizePolicy();
        policy.setHorizontalStretch(stretch);
        pane->setSizePolicy(policy);
        splitter.addWidget(pane);
    }
    splitter.resize(400, 100);
    ViewSetup::splitByStretch(&splitter);
    splitter.show();
    QVERIFY(QTest::qWaitForWindowExposed(&splitter));

    const QList<int> initial = splitter.sizes();
    splitter.resize(800, 100);
    QTRY_COMPARE(splitter.sizes().at(1), initial.at(1));
    QVERIFY(splitter.sizes().at(0) > initial.at(0));
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

namespace {

VariableInfo speedVariable()
{
    VariableInfo variable;
    variable.nodeId = QStringLiteral("ns=1;s=Motor.Speed");
    variable.displayName = QStringLiteral("Speed");
    variable.icon = QStringLiteral("signal");
    variable.dataType = QStringLiteral("Double");
    variable.access = QStringLiteral("Read/Write");
    variable.typeName = QStringLiteral("AnalogItemType");
    variable.initialValue = 1.5;
    variable.simulation.function = SimulationFunction::Sine;
    return variable;
}

VariableInfo rangeProperty()
{
    VariableInfo property;
    property.nodeId = QStringLiteral("ns=1;s=Motor.Speed.EURange");
    property.displayName = QStringLiteral("EURange");
    property.dataType = QStringLiteral("Range");
    property.access = QStringLiteral("Read");
    property.typeName = QStringLiteral("PropertyType");
    property.initialValue = QStringLiteral("0; 100");
    property.property = true;
    property.fromModel = true;
    return property;
}

template <typename T>
T *named(const QWidget &parent, const char *name)
{
    T *widget = parent.findChild<T *>(QString::fromLatin1(name));
    Q_ASSERT(widget);
    return widget;
}

}

void TestWidgets::variablePropertiesEditsVariable()
{
    VariablePropertiesWidget editor;
    QVERIFY2(!named<QWidget>(editor, "propertiesActions")->isEnabled(), "an empty editor applies nothing");
    editor.setVariable(speedVariable());
    QCOMPARE(named<QLineEdit>(editor, "nodeIdEdit")->text(), QStringLiteral("ns=1;s=Motor.Speed"));
    QCOMPARE(named<QLineEdit>(editor, "typeEdit")->text(), QStringLiteral("AnalogItemType"));
    QCOMPARE(named<QLineEdit>(editor, "initialValueEdit")->text(), Formatting::value(1.5));

    QSignalSpy applied(&editor, &VariablePropertiesWidget::applyRequested);
    QSignalSpy alarm(&editor, &VariablePropertiesWidget::alarmEditRequested);
    named<QLineEdit>(editor, "displayNameEdit")->setText(QStringLiteral(" Rpm "));
    named<QPushButton>(editor, "applyButton")->click();
    QCOMPARE(applied.count(), 1);
    const VariableInfo edited = applied.first().first().value<VariableInfo>();
    QCOMPARE(edited.displayName, QStringLiteral("Rpm"));
    QVERIFY2(!edited.initialValue.isValid(), "an untouched initial value is kept");
    named<QPushButton>(editor, "alarmButton")->click();
    QCOMPARE(alarm.count(), 1);
    QCOMPARE(alarm.first().first().value<VariableInfo>().displayName, QStringLiteral("Speed"));

    named<QPushButton>(editor, "revertButton")->click();
    QCOMPARE(named<QLineEdit>(editor, "displayNameEdit")->text(), QStringLiteral("Speed"));
    editor.clear();
    QVERIFY(named<QLineEdit>(editor, "nodeIdEdit")->text().isEmpty());
    named<QPushButton>(editor, "applyButton")->click();
    QCOMPARE(applied.count(), 1);
}

void TestWidgets::variablePropertiesShowsCurrentQuality_data()
{
    QTest::addColumn<VariableInfo::Quality>("quality");
    QTest::addColumn<VariableInfo::Quality>("configuredQuality");
    for (VariableInfo::Quality quality : VariableInfo::qualities())
        QTest::newRow(qPrintable(VariablesModel::qualityText(quality))) << quality << quality;
    QTest::newRow("waiting for initial data") << VariableInfo::Quality::BadWaitingForInitialData
                                              << VariableInfo::Quality::Good;
    QTest::newRow("server failure") << VariableInfo::Quality::Bad << VariableInfo::Quality::Uncertain;
}

void TestWidgets::variablePropertiesShowsCurrentQuality()
{
    QFETCH(VariableInfo::Quality, quality);
    QFETCH(VariableInfo::Quality, configuredQuality);
    VariablePropertiesWidget editor;
    auto *indicator = editor.findChild<StatusIndicator *>(QStringLiteral("qualityIndicator"));
    QVERIFY(indicator);
    QCOMPARE(indicator->status(), StatusKind::None);
    QVERIFY(indicator->text().isEmpty());

    VariableInfo variable = speedVariable();
    variable.quality = quality;
    variable.configuredQuality = configuredQuality;
    editor.setVariable(variable);
    VariablesModel model;
    model.setItems({ variable });
    const QModelIndex cell = model.index(0, VariablesModel::ColQuality);
    QCOMPARE(indicator->text(), cell.data().toString());
    QCOMPARE(indicator->status(), VariablesModel::qualityStatus(quality));
    auto *combo = named<QComboBox>(editor, "qualityCombo");
    QCOMPARE(combo->currentData().toUInt(), quint32(configuredQuality));
    QCOMPARE(named<QLabel>(editor, "configuredQualityLabel")->text(), QStringLiteral("Configured Quality"));

    combo->setCurrentIndex(combo->findData(quint32(VariableInfo::Quality::BadSensorFailure)));
    QEvent languageChange(QEvent::LanguageChange);
    QCoreApplication::sendEvent(&editor, &languageChange);
    QCOMPARE(indicator->text(), cell.data().toString());
    QCOMPARE(combo->currentData().toUInt(), quint32(VariableInfo::Quality::BadSensorFailure));
    QSignalSpy applied(&editor, &VariablePropertiesWidget::applyRequested);
    named<QCheckBox>(editor, "historizingCheck")->toggle();
    named<QPushButton>(editor, "applyButton")->click();
    QCOMPARE(applied.count(), 1);
    const VariableInfo edited = applied.first().first().value<VariableInfo>();
    QCOMPARE(edited.quality, quality);
    QCOMPARE(edited.configuredQuality, VariableInfo::Quality::BadSensorFailure);

    named<QPushButton>(editor, "revertButton")->click();
    QCOMPARE(indicator->text(), cell.data().toString());
    QCOMPARE(combo->currentData().toUInt(), quint32(configuredQuality));
    variable.quality = VariableInfo::Quality::UncertainLastUsableValue;
    editor.setVariable(variable);
    QCOMPARE(indicator->text(), VariablesModel::qualityText(variable.quality));
    QCOMPARE(indicator->status(), StatusKind::Warning);
    editor.clear();
    QCOMPARE(indicator->status(), StatusKind::None);
    QVERIFY(indicator->text().isEmpty());
    QCoreApplication::sendEvent(&editor, &languageChange);
    QVERIFY(indicator->text().isEmpty());
}

void TestWidgets::variablePropertiesLocksReadOnlyAndModelVariables()
{
    VariablePropertiesWidget editor;
    VariableInfo variable = speedVariable();
    variable.fromModel = true;
    editor.setVariable(variable);
    QVERIFY(named<QWidget>(editor, "propertiesHost")->isEnabled());
    QVERIFY(!named<QWidget>(editor, "displayNameEdit")->isEnabled());
    QVERIFY(!named<QWidget>(editor, "dataTypeCombo")->isEnabled());
    QVERIFY(named<QWidget>(editor, "simulationCombo")->isEnabled());

    variable.readOnly = true;
    editor.setVariable(variable);
    QVERIFY(!named<QWidget>(editor, "propertiesHost")->isEnabled());
    QVERIFY(!named<QWidget>(editor, "propertiesActions")->isEnabled());
}

void TestWidgets::propertyNodeEditsValue()
{
    PropertyNodeWidget editor;
    QVERIFY(!named<QWidget>(editor, "propertyNodeActions")->isEnabled());
    editor.setNode(rangeProperty());
    QCOMPARE(named<QLineEdit>(editor, "propertyNodeIdEdit")->text(), QStringLiteral("ns=1;s=Motor.Speed.EURange"));
    QCOMPARE(named<QLineEdit>(editor, "propertyTypeEdit")->text(), QStringLiteral("PropertyType"));
    QCOMPARE(named<QLineEdit>(editor, "propertyNameEdit")->text(), QStringLiteral("EURange"));
    QCOMPARE(named<QLineEdit>(editor, "propertyDataTypeEdit")->text(), QStringLiteral("Range"));
    QCOMPARE(named<QLineEdit>(editor, "propertyAccessEdit")->text(), QStringLiteral("Read"));
    QVERIFY2(named<QLineEdit>(editor, "propertyValueEdit")->isHidden(), "a Range is edited as its limits");
    auto *low = named<QLineEdit>(editor, "propertyLowEdit");
    auto *high = named<QLineEdit>(editor, "propertyHighEdit");
    QVERIFY(!low->isHidden());
    QVERIFY(!high->isHidden());
    QCOMPARE(low->text(), QStringLiteral("0"));
    QCOMPARE(high->text(), QStringLiteral("100"));
    QVERIFY(!low->isReadOnly());
    for (const char *name : { "propertyNodeIdEdit", "propertyTypeEdit", "propertyNameEdit", "propertyDataTypeEdit",
                              "propertyAccessEdit" })
        QVERIFY2(named<QLineEdit>(editor, name)->isReadOnly(), name);

    QSignalSpy applied(&editor, &PropertyNodeWidget::applyRequested);
    named<QPushButton>(editor, "propertyApplyButton")->click();
    QVERIFY2(applied.isEmpty(), "nothing typed, nothing applied");

    low->setText(QStringLiteral("-10.5"));
    low->setModified(true);
    named<QPushButton>(editor, "propertyApplyButton")->click();
    QCOMPARE(applied.last().first().value<VariableInfo>().initialValue.toString(), QStringLiteral("-10.5; 100"));
    QString point = QStringLiteral("1.5");
    int position = 0;
    QCOMPARE(low->validator()->validate(point, position), QValidator::Acceptable);

    named<QPushButton>(editor, "propertyRevertButton")->click();
    QCOMPARE(low->text(), QStringLiteral("0"));
    QVERIFY(!low->isModified());
}

void TestWidgets::variablePropertiesEnablesApplyOnEdit_data()
{
    QTest::addColumn<QString>("editor");
    for (const char *name : { "displayNameEdit", "iconCombo", "arrayLengthSpin", "amplitudeSpin", "historizingCheck" })
        QTest::newRow(name) << QString::fromLatin1(name);
}

void TestWidgets::variablePropertiesEnablesApplyOnEdit()
{
    QFETCH(QString, editor);
    VariablePropertiesWidget properties;
    auto *apply = named<QPushButton>(properties, "applyButton");
    QVERIFY(!apply->isEnabled());
    properties.setVariable(speedVariable());
    QVERIFY(!apply->isEnabled());

    QWidget *edited = properties.findChild<QWidget *>(editor);
    if (auto *edit = qobject_cast<QLineEdit *>(edited))
        edit->setText(QStringLiteral("Rpm"));
    else if (auto *combo = qobject_cast<QComboBox *>(edited))
        combo->setCurrentIndex(combo->currentIndex() == 0 ? 1 : 0);
    else if (auto *spin = qobject_cast<QSpinBox *>(edited))
        spin->setValue(spin->value() + 1);
    else if (auto *doubleSpin = qobject_cast<QDoubleSpinBox *>(edited))
        doubleSpin->setValue(doubleSpin->value() + 1);
    else if (auto *check = qobject_cast<QCheckBox *>(edited))
        check->toggle();
    QVERIFY(apply->isEnabled());

    named<QPushButton>(properties, "revertButton")->click();
    QVERIFY(!apply->isEnabled());
    named<QCheckBox>(properties, "historizingCheck")->toggle();
    properties.setVariable(speedVariable());
    QVERIFY2(!apply->isEnabled(), "showing the stored variable after Apply leaves nothing to apply");
}

void TestWidgets::propertyNodeEnablesApplyOnEdit()
{
    PropertyNodeWidget editor;
    auto *apply = named<QPushButton>(editor, "propertyApplyButton");
    editor.setNode(rangeProperty());
    QVERIFY(!apply->isEnabled());

    named<QLineEdit>(editor, "propertyLowEdit")->setText(QStringLiteral("-1"));
    QVERIFY(apply->isEnabled());
    named<QPushButton>(editor, "propertyRevertButton")->click();
    QVERIFY(!apply->isEnabled());

    named<QLineEdit>(editor, "propertyHighEdit")->setText(QStringLiteral("50"));
    editor.setNode(rangeProperty());
    QVERIFY(!apply->isEnabled());
}

void TestWidgets::propertyNodeEditsTextValue()
{
    PropertyNodeWidget editor;
    VariableInfo units = rangeProperty();
    units.displayName = QStringLiteral("EngineeringUnits");
    units.dataType = QStringLiteral("EUInformation");
    units.initialValue = QStringLiteral("rpm");
    editor.setNode(units);
    QVERIFY(named<QLineEdit>(editor, "propertyLowEdit")->isHidden());
    QVERIFY(named<QLineEdit>(editor, "propertyHighEdit")->isHidden());
    auto *value = named<QLineEdit>(editor, "propertyValueEdit");
    QVERIFY(!value->isHidden());
    QCOMPARE(value->text(), QStringLiteral("rpm"));

    QSignalSpy applied(&editor, &PropertyNodeWidget::applyRequested);
    value->setText(QStringLiteral("Hz"));
    value->setModified(true);
    named<QPushButton>(editor, "propertyApplyButton")->click();
    QCOMPARE(applied.count(), 1);
    QCOMPARE(applied.first().first().value<VariableInfo>().initialValue.toString(), QStringLiteral("Hz"));
}

void TestWidgets::propertyNodeLocksReadOnlyProperty()
{
    PropertyNodeWidget editor;
    VariableInfo property = rangeProperty();
    property.readOnly = true;
    editor.setNode(property);
    QVERIFY(named<QLineEdit>(editor, "propertyLowEdit")->isReadOnly());
    QVERIFY(named<QLineEdit>(editor, "propertyHighEdit")->isReadOnly());
    QVERIFY(!named<QWidget>(editor, "propertyNodeActions")->isEnabled());
    editor.clear();
    QVERIFY(named<QLineEdit>(editor, "propertyNodeIdEdit")->text().isEmpty());
}

QTEST_MAIN(TestWidgets)
#include "test_widgets.moc"
