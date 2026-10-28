// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QAction>
#include <QDialog>
#include <QHeaderView>
#include <QPushButton>
#include <QScreen>
#include <QSplitter>
#include <QTabWidget>
#include <QTableView>
#include <QTimer>
#include <QtTest>

#include "mainwindow.h"
#include "messagedialog.h"
#include "mock/mockdata.h"
#include "project/projectdocument.h"
#include "server/servercontroller.h"
#include "settingsstore.h"
#include "widgetlayout.h"

namespace {

///
/// \brief Finds a named UI control required by a layout regression test.
///
template<typename T>
T *control(const QWidget &root, const char *name)
{
    auto *widget = root.findChild<T *>(QLatin1String(name));
    if (!widget)
        qFatal("Missing control %s", name);
    return widget;
}

const QString kNamespacesSplitterKey = QStringLiteral(
    "ui/mainWindow/splitters/centralWidget/contentSplitter/tabWidget/qt_tabwidget_stackedwidget/namespacesPage/splitter");

///
/// \brief Returns the namespaces page split of a first start without saved settings, at 1000x700.
///
QList<int> defaultNamespacesSplit()
{
    MainWindow window;
    window.resize(1000, 700);
    window.show();
    if (!QTest::qWaitForWindowExposed(&window))
        return {};
    control<QTabWidget>(window, "tabWidget")->setCurrentWidget(control<QWidget>(window, "namespacesPage"));
    QCoreApplication::processEvents();
    return control<QWidget>(window, "namespacesPage")->findChild<QSplitter *>()->sizes();
}

///
/// \brief Tells whether two splits differ by at most 2.5% of the width in every pane; the default split
///        itself shifts a little with the window sizes the page went through before it was shown.
///
bool similarSplit(const QList<int> &actual, const QList<int> &expected)
{
    if (actual.size() != expected.size())
        return false;
    const int tolerance = std::accumulate(expected.cbegin(), expected.cend(), 0) / 40;
    for (qsizetype i = 0; i < actual.size(); ++i) {
        if (qAbs(actual.at(i) - expected.at(i)) > tolerance)
            return false;
    }
    return true;
}

}

class TestWindowLayout : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void geometry_data();
    void geometry();
    void splitters();
    void unvisitedPagesKeepDefaultSplit();
    void unscaledSplitOfCurrentPageKeepsRatio();
    void columnsSurviveLateData();
    void navigation_data();
    void navigation();
    void invalidSettingsKeepDefaults();
    void cancelledCloseKeepsSavedLayout();
    void childWindowsAreExcluded();
};

///
/// \brief Keeps layout settings independent between test cases.
///
void TestWindowLayout::init()
{
    SettingsStore().remove(QStringLiteral("ui/mainWindow"));
}

///
/// \brief Covers normal, maximized and minimized application exits.
///
void TestWindowLayout::geometry_data()
{
    QTest::addColumn<int>("state");
    QTest::newRow("normal") << int(Qt::WindowNoState);
    QTest::newRow("maximized") << int(Qt::WindowMaximized);
    QTest::newRow("minimized") << int(Qt::WindowMinimized);
}

///
/// \brief Restores normal geometry and maximization without reopening minimized.
///
void TestWindowLayout::geometry()
{
    QFETCH(int, state);
    QRect expected;
    {
        MainWindow window;
        window.resize(1000, 680);
        window.move(40, 50);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        expected = window.geometry();
        window.setWindowState(Qt::WindowStates(state));
        QCoreApplication::processEvents();
        QVERIFY(window.close());
    }
    MainWindow restored;
    restored.show();
    QVERIFY(QTest::qWaitForWindowExposed(&restored));
    QCOMPARE(restored.isMaximized(), state == Qt::WindowMaximized);
    QVERIFY(!restored.isMinimized());
    if (restored.isMaximized()) {
        restored.showNormal();
        QCoreApplication::processEvents();
    }
    QCOMPARE(restored.size(), expected.size());
    const QRect screen = restored.screen()->availableGeometry();
    if (screen.contains(expected))
        QCOMPARE(restored.geometry(), expected);
    else
        QVERIFY(screen.intersects(restored.frameGeometry()));
}

///
/// \brief Restores independent divider positions on every page and in the event log.
///
void TestWindowLayout::splitters()
{
    QMap<QString, QList<int>> expected;
    QList<int> contentSizes;
    {
        MainWindow window;
        window.resize(1000, 700);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto *tabs = control<QTabWidget>(window, "tabWidget");
        for (int i = 0; i < tabs->count(); ++i) {
            tabs->setCurrentIndex(i);
            QCoreApplication::processEvents();
            auto *splitter = tabs->widget(i)->findChild<QSplitter *>();
            if (!splitter)
                continue;
            QList<int> sizes = splitter->sizes();
            sizes.first() += 47 + i;
            sizes.last() -= 47 + i;
            splitter->setSizes(sizes);
            expected.insert(tabs->widget(i)->objectName(), splitter->sizes());
        }
        tabs->setCurrentIndex(1);
        QCoreApplication::processEvents();
        auto *content = control<QSplitter>(window, "contentSplitter");
        content->setSizes({500, 260});
        contentSizes = content->sizes();
        QVERIFY(window.close());
    }
    MainWindow restored;
    restored.show();
    QVERIFY(QTest::qWaitForWindowExposed(&restored));
    QCOMPARE(control<QSplitter>(restored, "contentSplitter")->sizes(), contentSizes);
    auto *tabs = control<QTabWidget>(restored, "tabWidget");
    for (int i = 0; i < tabs->count(); ++i) {
        tabs->setCurrentIndex(i);
        QCoreApplication::processEvents();
        auto *splitter = tabs->widget(i)->findChild<QSplitter *>();
        if (splitter)
            QCOMPARE(splitter->sizes(), expected.value(tabs->widget(i)->objectName()));
    }
}

///
/// \brief Opens pages that were never shown before the last exit with the same split as a first start.
///
void TestWindowLayout::unvisitedPagesKeepDefaultSplit()
{
    const QList<int> defaults = defaultNamespacesSplit();
    {
        MainWindow window;
        window.resize(1000, 700);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        control<QTabWidget>(window, "tabWidget")->setCurrentIndex(0);
        QCoreApplication::processEvents();
        QVERIFY(window.close());
    }
    QVERIFY(!SettingsStore().contains(kNamespacesSplitterKey));

    SettingsStore().setValue(QStringLiteral("ui/mainWindow/currentPage"), QStringLiteral("namespacesPage"));
    MainWindow restored;
    restored.show();
    QVERIFY(QTest::qWaitForWindowExposed(&restored));
    const QList<int> sizes = control<QWidget>(restored, "namespacesPage")->findChild<QSplitter *>()->sizes();
    QVERIFY2(similarSplit(sizes, defaults), qPrintable(QDebug::toString(sizes)));
}

///
/// \brief Scales the unscaled initial sizes earlier versions saved for never shown pages instead of
///        squeezing them into the first pane of the page shown at start.
///
void TestWindowLayout::unscaledSplitOfCurrentPageKeepsRatio()
{
    const QList<int> defaults = defaultNamespacesSplit();
    QSplitter unscaled;
    unscaled.addWidget(new QWidget);
    unscaled.addWidget(new QWidget);
    unscaled.setSizes({ 12000, 5800 });
    {
        SettingsStore settings;
        settings.setValue(kNamespacesSplitterKey, unscaled.saveState());
        settings.setValue(QStringLiteral("ui/mainWindow/currentPage"), QStringLiteral("namespacesPage"));
    }

    MainWindow restored;
    restored.resize(1000, 700);
    restored.show();
    QVERIFY(QTest::qWaitForWindowExposed(&restored));
    const QList<int> sizes = control<QWidget>(restored, "namespacesPage")->findChild<QSplitter *>()->sizes();
    QVERIFY2(similarSplit(sizes, defaults), qPrintable(QDebug::toString(sizes)));
}

///
/// \brief Retains separate log column widths and user column order after models receive their first rows.
///
void TestWindowLayout::columnsSurviveLateData()
{
    {
        MainWindow window;
        window.setSnapshot(MockData::snapshot());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        control<QTableView>(*control<QWidget>(window, "eventLog"), "logView")->setColumnWidth(0, 241);
        control<QTableView>(*control<QWidget>(window, "logPage"), "logView")->setColumnWidth(0, 287);
        auto *header = control<QTableView>(window, "endpointsView")->horizontalHeader();
        header->resizeSection(0, 81);
        header->moveSection(0, 1);
        QVERIFY(window.close());
    }
    MainWindow restored;
    restored.show();
    QVERIFY(QTest::qWaitForWindowExposed(&restored));
    restored.setSnapshot(MockData::snapshot());
    QCoreApplication::processEvents();
    QCOMPARE(control<QTableView>(*control<QWidget>(restored, "eventLog"), "logView")->columnWidth(0), 241);
    QCOMPARE(control<QTableView>(*control<QWidget>(restored, "logPage"), "logView")->columnWidth(0), 287);
    auto *tabs = control<QTabWidget>(restored, "tabWidget");
    tabs->setCurrentWidget(control<QWidget>(restored, "endpointsPage"));
    QCoreApplication::processEvents();
    auto *header = control<QTableView>(restored, "endpointsView")->horizontalHeader();
    QCOMPARE(header->sectionSize(0), 81);
    QCOMPARE(header->visualIndex(0), 1);
    restored.setSnapshot(MockData::snapshot());
    QCOMPARE(header->sectionSize(0), 81);
}

///
/// \brief Covers a manually hidden event log and its automatic hiding on the Log page.
///
void TestWindowLayout::navigation_data()
{
    QTest::addColumn<int>("page");
    QTest::addColumn<bool>("logVisible");
    QTest::newRow("objects-with-log") << 1 << true;
    QTest::newRow("types-without-log") << 2 << false;
    QTest::newRow("log-page") << 7 << true;
}

///
/// \brief Restores the page and the user's event log visibility preference independently.
///
void TestWindowLayout::navigation()
{
    QFETCH(int, page);
    QFETCH(bool, logVisible);
    {
        MainWindow window;
        window.show();
        control<QTabWidget>(window, "tabWidget")->setCurrentIndex(page);
        control<QAction>(window, "actionEventLog")->setChecked(logVisible);
        QVERIFY(window.close());
    }
    MainWindow restored;
    restored.show();
    QVERIFY(QTest::qWaitForWindowExposed(&restored));
    auto *tabs = control<QTabWidget>(restored, "tabWidget");
    QCOMPARE(tabs->currentIndex(), page);
    QCOMPARE(control<QAction>(restored, "actionEventLog")->isChecked(), logVisible);
    QCOMPARE(control<QWidget>(restored, "eventLog")->isVisible(), logVisible && page != 7);
    tabs->setCurrentIndex(0);
    QCOMPARE(control<QWidget>(restored, "eventLog")->isVisible(), logVisible);
}

///
/// \brief Ignores obsolete pages and malformed geometry, splitter and header data.
///
void TestWindowLayout::invalidSettingsKeepDefaults()
{
    MainWindow defaults;
    defaults.show();
    QVERIFY(QTest::qWaitForWindowExposed(&defaults));
    const auto contentSizes = control<QSplitter>(defaults, "contentSplitter")->sizes();
    const int columnWidth = control<QTableView>(defaults, "endpointsView")->columnWidth(0);
    {
        SettingsStore settings;
        settings.beginGroup(QStringLiteral("ui/mainWindow"));
        WidgetLayout::save(defaults, settings);
        for (const QString &key : settings.allKeys())
            settings.setValue(key, QByteArray("invalid"));
        settings.setValue(QStringLiteral("geometry"), QByteArray("invalid"));
        settings.setValue(QStringLiteral("state"), QByteArray("invalid"));
        settings.setValue(QStringLiteral("currentPage"), QStringLiteral("removedPage"));
    }
    MainWindow restored;
    restored.show();
    QVERIFY(QTest::qWaitForWindowExposed(&restored));
    QCOMPARE(restored.size(), defaults.size());
    QCOMPARE(control<QTabWidget>(restored, "tabWidget")->currentIndex(), 0);
    QCOMPARE(control<QSplitter>(restored, "contentSplitter")->sizes(), contentSizes);
    QCOMPARE(control<QTableView>(restored, "endpointsView")->columnWidth(0), columnWidth);
    restored.setSnapshot(MockData::snapshot());
    QVERIFY(control<QTableView>(restored, "endpointsView")->columnWidth(0) != columnWidth);
}

///
/// \brief Leaves the last accepted layout intact when unsaved project changes prevent closing.
///
void TestWindowLayout::cancelledCloseKeepsSavedLayout()
{
    ServerController controller;
    ProjectDocument document(&controller);
    document.newProject();
    MainWindow window;
    window.setController(&controller);
    window.setDocument(&document);
    window.show();
    QVERIFY(controller.removeNode(QStringLiteral("ns=1;s=Pump_01")));
    const QByteArray sentinel("last-accepted-geometry");
    SettingsStore().setValue(QStringLiteral("ui/mainWindow/geometry"), sentinel);
    QTimer::singleShot(0, &window, [&window] {
        control<QPushButton>(*window.findChild<MessageDialog *>(), "cancelButton")->click();
    });
    QVERIFY(!window.close());
    QVERIFY(window.isVisible());
    QCOMPARE(SettingsStore().value(QStringLiteral("ui/mainWindow/geometry")).toByteArray(), sentinel);
}

///
/// \brief Does not persist table state from temporary dialogs parented to the main window.
///
void TestWindowLayout::childWindowsAreExcluded()
{
    MainWindow window;
    QDialog dialog(&window);
    dialog.setObjectName(QStringLiteral("temporaryDialog"));
    QTableView view(&dialog);
    view.setObjectName(QStringLiteral("temporaryView"));
    SettingsStore settings;
    settings.beginGroup(QStringLiteral("ui/mainWindow"));
    WidgetLayout::save(window, settings);
    for (const QString &key : settings.allKeys())
        QVERIFY(!key.contains(QStringLiteral("temporary")));
}

QTEST_MAIN(TestWindowLayout)
#include "test_windowlayout.moc"
