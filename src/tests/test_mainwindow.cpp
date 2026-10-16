// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QAction>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QSignalSpy>
#include <QHeaderView>
#include <QSplitter>
#include <QTabWidget>
#include <QTableView>
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
    void menuBarListsMenus();
    void viewMenuSwitchesPages();
    void eventLogActionTogglesPanel();
    void toolbarButtonsTriggerMenuActions();
    void serverActionsFollowRunningState();
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
    QCOMPARE(window.size(), QSize(1331, 771));
}

void TestMainWindow::showsSnapshot()
{
    const ServerSnapshot snapshot = MockData::snapshot();
    MainWindow window;
    window.setSnapshot(snapshot);

    QCOMPARE(child<QLineEdit>(window, "endpointEdit")->text(), snapshot.status.primaryEndpoint);
    const QList<StatusIndicator *> indicators
        = window.findChildren<StatusIndicator *>(QStringLiteral("serverIndicator"));
    QCOMPARE(indicators.size(), 1);
    QVERIFY(qobject_cast<MainStatusBarWidget *>(indicators.first()->parentWidget()));
    QCOMPARE(indicators.first()->status(), StatusKind::Good);
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

    const auto *logView = child<EventLogWidget>(window, "eventLog")->findChild<QTableView *>();
    QCOMPARE(logView->viewport()->height() % logView->verticalHeader()->defaultSectionSize(), 0);
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

    QVERIFY(bar->trailingText().isEmpty());
    tabs->setCurrentWidget(child<QWidget>(window, "logPage"));
    QCOMPARE(bar->trailingText(), QStringLiteral("Log entries: %1").arg(snapshot.log.size()));

    window.logModel()->clear();
    QCOMPARE(bar->trailingText(), QStringLiteral("Log entries: 0"));

    tabs->setCurrentIndex(0);
    QVERIFY(bar->trailingText().isEmpty());
}

void TestMainWindow::themeButtonHiddenWithoutTheme()
{
    MainWindow window;
    QVERIFY(child<QToolButton>(window, "themeButton")->isHidden());
    QVERIFY(!child<QMenu>(window, "menuTheme")->menuAction()->isVisible());
}

void TestMainWindow::themeMenuSwitchesTheme()
{
    AppTheme theme;
    theme.setMode(AppTheme::Mode::Light);
    MainWindow window(&theme);

    auto *button = child<QToolButton>(window, "themeButton");
    QVERIFY(!button->isHidden());
    QCOMPARE(button->menu(), child<QMenu>(window, "menuTheme"));
    QVERIFY(button->menu()->menuAction()->isVisible());
    QCOMPARE(button->menu()->actions().size(), 3);
    QVERIFY(child<QAction>(window, "actionThemeLight")->isChecked());

    child<QAction>(window, "actionThemeDark")->trigger();
    QCOMPARE(theme.mode(), AppTheme::Mode::Dark);
    QVERIFY(AppPalette::isDark(QApplication::palette()));
    QVERIFY(!child<QAction>(window, "actionThemeLight")->isChecked());

    child<QAction>(window, "actionThemeSystem")->trigger();
    QCOMPARE(theme.mode(), AppTheme::Mode::System);
}

void TestMainWindow::menuBarListsMenus()
{
    MainWindow window;
    QStringList titles;
    for (const QAction *action : window.menuBar()->actions())
        titles.append(action->text());
    QCOMPARE(titles, QStringList({ QStringLiteral("&File"), QStringLiteral("&Server"),
                                   QStringLiteral("&View"), QStringLiteral("&Help") }));
}

void TestMainWindow::viewMenuSwitchesPages()
{
    MainWindow window;
    auto *tabs = child<QTabWidget>(window, "tabWidget");
    const QList<QAction *> actions = child<QMenu>(window, "menuView")->actions().mid(0, tabs->count());

    for (int i = 0; i < tabs->count(); ++i) {
        QCOMPARE(actions.at(i)->text(), tabs->tabText(i));
        QCOMPARE(actions.at(i)->shortcut(), QKeySequence(Qt::CTRL | Qt::Key(Qt::Key_1 + i)));
    }
    QVERIFY(actions.at(0)->isChecked());

    actions.at(2)->trigger();
    QCOMPARE(tabs->currentIndex(), 2);

    tabs->setCurrentIndex(5);
    QVERIFY(actions.at(5)->isChecked());
    QVERIFY(!actions.at(2)->isChecked());
}

void TestMainWindow::eventLogActionTogglesPanel()
{
    MainWindow window;
    window.show();
    auto *eventLog = child<EventLogWidget>(window, "eventLog");
    auto *action = child<QAction>(window, "actionEventLog");
    auto *tabs = child<QTabWidget>(window, "tabWidget");

    action->setChecked(false);
    QVERIFY(!eventLog->isVisible());
    tabs->setCurrentIndex(1);
    QVERIFY(!eventLog->isVisible());

    action->setChecked(true);
    QVERIFY(eventLog->isVisible());

    tabs->setCurrentWidget(child<QWidget>(window, "logPage"));
    QVERIFY(!eventLog->isVisible());
    QVERIFY(!action->isEnabled());
}

void TestMainWindow::toolbarButtonsTriggerMenuActions()
{
    MainWindow window;
    const QList<QPair<const char *, const char *>> pairs = {
        { "addNodeButton", "actionAddNode" },
        { "importModelButton", "actionImportModel" },
        { "saveButton", "actionSaveProject" },
    };
    for (const auto &pair : pairs) {
        QSignalSpy triggered(child<QAction>(window, pair.second), &QAction::triggered);
        child<QPushButton>(window, pair.first)->click();
        QVERIFY2(triggered.count() == 1, pair.first);
    }
}

void TestMainWindow::serverActionsFollowRunningState()
{
    ServerSnapshot snapshot = MockData::snapshot();
    snapshot.status.running = true;
    MainWindow window;
    window.setSnapshot(snapshot);

    QSignalSpy stopped(child<QAction>(window, "actionStopServer"), &QAction::triggered);
    child<QPushButton>(window, "stopButton")->click();
    QCOMPARE(stopped.count(), 1);
    QVERIFY(!child<QAction>(window, "actionStartServer")->isEnabled());
    QVERIFY(child<QAction>(window, "actionRestartServer")->isEnabled());

    snapshot.status.running = false;
    window.setSnapshot(snapshot);
    QVERIFY(child<QPushButton>(window, "startButton")->isEnabled());
    QVERIFY(!child<QPushButton>(window, "stopButton")->isEnabled());
    QVERIFY(!child<QAction>(window, "actionStopServer")->isEnabled());
    QVERIFY(!child<QAction>(window, "actionRestartServer")->isEnabled());
}

QTEST_MAIN(TestMainWindow)
#include "test_mainwindow.moc"
