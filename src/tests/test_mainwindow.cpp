// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QAction>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSplitter>
#include <QTabWidget>
#include <QTest>
#include <QToolButton>

#include "apppalette.h"
#include "apptheme.h"
#include "eventlogwidget.h"
#include "logmodel.h"
#include "mainstatusbarwidget.h"
#include "mainwindow.h"
#include "mock/mockdata.h"
#include "statusindicator.h"

namespace {

template <typename T>
T *child(const QWidget &parent, const char *name)
{
    T *found = parent.findChild<T *>(QLatin1String(name));
    if (!found)
        qFatal("Missing child %s", name);
    return found;
}

}

class TestMainWindow : public QObject
{
    Q_OBJECT

private slots:
    void hasAllPages();
    void showsSnapshot();
    void eventLogFollowsTab();
    void eventLogGetsItsShareOnFirstShow();
    void pagesDoNotBlockTheEventLog();
    void statusBarNoteFollowsTab();
    void themeButtonHiddenWithoutTheme();
    void themeMenuSwitchesTheme();
};

void TestMainWindow::hasAllPages()
{
    MainWindow window;
    const auto *tabs = child<QTabWidget>(window, "tabWidget");
    const QStringList expected = { QStringLiteral("Status"), QStringLiteral("Objects"), QStringLiteral("Types"),
                                   QStringLiteral("Namespaces"), QStringLiteral("Endpoints"),
                                   QStringLiteral("Certificates"), QStringLiteral("Sessions"), QStringLiteral("Log") };
    QCOMPARE(tabs->count(), expected.size());
    for (int i = 0; i < expected.size(); ++i)
        QCOMPARE(tabs->tabText(i), expected.at(i));
    QCOMPARE(window.windowTitle(), QStringLiteral("OpenUaSim"));
    QCOMPARE(window.size(), QSize(1331, 750));
}

void TestMainWindow::showsSnapshot()
{
    const ServerSnapshot snapshot = MockData::snapshot();
    MainWindow window;
    window.setSnapshot(snapshot);

    QCOMPARE(child<QLineEdit>(window, "endpointEdit")->text(), snapshot.status.primaryEndpoint);
    QCOMPARE(child<StatusIndicator>(window, "serverIndicator")->status(), StatusKind::Good);
    QVERIFY(!child<QPushButton>(window, "startButton")->isEnabled());
    QVERIFY(child<QPushButton>(window, "stopButton")->isEnabled());
    QCOMPARE(window.logModel()->rowCount(), snapshot.log.size());
}

void TestMainWindow::eventLogFollowsTab()
{
    MainWindow window;
    window.setSnapshot(MockData::snapshot());
    window.show();
    auto *tabs = child<QTabWidget>(window, "tabWidget");
    auto *eventLog = child<EventLogWidget>(window, "eventLog");

    for (int index : { 0, 1 }) {
        tabs->setCurrentIndex(index);
        QVERIFY(eventLog->isVisible());
        QCOMPARE(eventLog->title(), QStringLiteral("Event Log"));
    }

    tabs->setCurrentWidget(child<QWidget>(window, "logPage"));
    QVERIFY(!eventLog->isVisible());
}

void TestMainWindow::eventLogGetsItsShareOnFirstShow()
{
    MainWindow window;
    window.setSnapshot(MockData::snapshot());
    window.resize(1068, 614);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    const QList<int> sizes = child<QSplitter>(window, "contentSplitter")->sizes();
    QCOMPARE(sizes.size(), 2);
    QVERIFY2(sizes.at(1) >= (sizes.at(0) + sizes.at(1)) / 4,
             qPrintable(QStringLiteral("pages %1, log %2").arg(sizes.at(0)).arg(sizes.at(1))));
}

void TestMainWindow::pagesDoNotBlockTheEventLog()
{
    MainWindow window;
    window.setSnapshot(MockData::snapshot());
    const auto *tabs = child<QTabWidget>(window, "tabWidget");
    for (int i = 0; i < tabs->count(); ++i) {
        QVERIFY2(tabs->widget(i)->minimumSizeHint().height() <= 300,
                 qPrintable(QStringLiteral("%1: %2").arg(tabs->tabText(i))
                                .arg(tabs->widget(i)->minimumSizeHint().height())));
    }
}

void TestMainWindow::statusBarNoteFollowsTab()
{
    const ServerSnapshot snapshot = MockData::snapshot();
    MainWindow window;
    window.setSnapshot(snapshot);
    auto *tabs = child<QTabWidget>(window, "tabWidget");
    auto *bar = window.findChild<MainStatusBarWidget *>();

    QCOMPARE(bar->trailingText(), QStringLiteral("Nodes: %1").arg(snapshot.status.totalNodes));
    tabs->setCurrentWidget(child<QWidget>(window, "logPage"));
    QCOMPARE(bar->trailingText(), QStringLiteral("Log entries: %1").arg(snapshot.log.size()));

    window.logModel()->clear();
    QCOMPARE(bar->trailingText(), QStringLiteral("Log entries: 0"));
}

void TestMainWindow::themeButtonHiddenWithoutTheme()
{
    MainWindow window;
    QVERIFY(child<QToolButton>(window, "themeButton")->isHidden());
}

void TestMainWindow::themeMenuSwitchesTheme()
{
    AppTheme theme;
    theme.setMode(AppTheme::Mode::Light);
    MainWindow window(&theme);

    auto *button = child<QToolButton>(window, "themeButton");
    QVERIFY(!button->isHidden());
    QVERIFY(button->menu());
    QCOMPARE(button->menu()->actions().size(), 3);
    QVERIFY(child<QAction>(window, "actionThemeLight")->isChecked());

    child<QAction>(window, "actionThemeDark")->trigger();
    QCOMPARE(theme.mode(), AppTheme::Mode::Dark);
    QVERIFY(AppPalette::isDark(QApplication::palette()));
    QVERIFY(!child<QAction>(window, "actionThemeLight")->isChecked());

    child<QAction>(window, "actionThemeSystem")->trigger();
    QCOMPARE(theme.mode(), AppTheme::Mode::System);
}

QTEST_MAIN(TestMainWindow)
#include "test_mainwindow.moc"
