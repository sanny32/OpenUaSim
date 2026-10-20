// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QAction>
#include <QFile>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QApplication>
#include <QFontDatabase>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalSpy>
#include <QHeaderView>
#include <QInputDialog>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QTableView>
#include <QTest>
#include <QToolButton>
#include <QTreeView>

#include "addnodedialog.h"
#include "alarmdialog.h"
#include "applanguage.h"
#include "apppalette.h"
#include "appstyle.h"
#include "apptheme.h"
#include "endpointspage.h"
#include "eventlogwidget.h"
#include "itemroles.h"
#include "logmodel.h"
#include "mainstatusbarwidget.h"
#include "mainwindow.h"
#include "mock/mockdata.h"
#include "objectspage.h"
#include "project/defaultproject.h"
#include "project/projectdocument.h"
#include "project/projectnodes.h"
#include "server/servercontroller.h"
#include "serversettingsdialog.h"
#include "statusindicator.h"
#include "testport.h"
#include "usersdialog.h"
#include "viewsetup.h"

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
    void statusRefreshKeepsEndpointSelection();
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
    void controllerDrivesWindow();
    void startFailureShowsMessage();
    void startWarningShowsMessage();
    void addNodeThroughDialog();
    void deleteNodeAfterConfirmation();
    void deleteNodeFollowsSelection();
    void duplicatesAndPastesNodes();
    void deletesSelectedNodes();
    void renamesNodeFromDialog();
    void switchesLanguageWithoutRestart();
    void configuresAlarmFromDialog();
    void appliesUsersFromDialog();
    void appliesServerSettingsFromDialog();
    void refusedVariableChangeShowsMessage();
    void endpointChangesShowRestartNote();
    void refusedEndpointChangeShowsMessage();
    void titleShowsDocumentAndChanges();
    void savesAndOpensProjectFiles();
    void openFailureShowsMessage();
    void closingAsksAboutUnsavedChanges();
    void importsAndExportsModels();
    void removesModelAfterConfirmation();
    void exportsNamespaceAndLog();
    void panelsFitTheDefaultWindow_data();
    void panelsFitTheDefaultWindow();
};

void TestMainWindow::hasAllPages()
{
    MainWindow window;
    const auto *tabs = child<QTabWidget>(window, "tabWidget");
    const QStringList expected = { QStringLiteral("Overview"), QStringLiteral("Objects"), QStringLiteral("Types"),
                                   QStringLiteral("Namespaces"), QStringLiteral("Endpoints"),
                                   QStringLiteral("Certificates"), QStringLiteral("Sessions"), QStringLiteral("Log") };
    QCOMPARE(tabs->count(), expected.size());
    for (int i = 0; i < expected.size(); ++i)
        QCOMPARE(tabs->tabText(i), expected.at(i));
    QCOMPARE(window.windowTitle(), QStringLiteral("OpenUaSim"));
    QCOMPARE(window.size(), QSize(1331, 811));
    QCOMPARE(window.minimumSize(), QSize(1000, 600));
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

void TestMainWindow::statusRefreshKeepsEndpointSelection()
{
    const ServerSnapshot snapshot = MockData::snapshot();
    MainWindow window;
    window.setSnapshot(snapshot);
    auto *endpoint = child<QLineEdit>(window, "endpointEdit");
    endpoint->selectAll();

    window.setSnapshot(snapshot);
    QCOMPARE(endpoint->selectedText(), snapshot.status.primaryEndpoint);
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

void TestMainWindow::controllerDrivesWindow()
{
    ProjectConfig project = DefaultProject::create();
    project.endpoints.first().port = TestPort::freePort();
    ServerController controller;
    controller.setProject(project);
    MainWindow window;
    window.setController(&controller);

    QCOMPARE(child<QLineEdit>(window, "endpointEdit")->text(), project.endpoints.first().url());
    const auto *variables = child<QTableView>(window, "variablesView");
    QCOMPARE(variables->model()->rowCount(), 3);
    const QString initial = variables->model()->index(0, 1).data().toString();

    child<QPushButton>(window, "startButton")->click();
    QVERIFY(controller.isRunning());
    QVERIFY(!child<QAction>(window, "actionStartServer")->isEnabled());
    QVERIFY(child<QAction>(window, "actionStopServer")->isEnabled());
    QTRY_VERIFY(variables->model()->index(0, 1).data().toString() != initial);
    QVERIFY(window.logModel()->rowCount() > 0);

    child<QAction>(window, "actionRestartServer")->trigger();
    QVERIFY(controller.isRunning());

    child<QPushButton>(window, "stopButton")->click();
    QVERIFY(!controller.isRunning());
    QVERIFY(child<QAction>(window, "actionStartServer")->isEnabled());
}

void TestMainWindow::startFailureShowsMessage()
{
    ProjectConfig project = DefaultProject::create();
    project.endpoints.clear();
    ServerController controller;
    controller.setProject(project);
    MainWindow window;
    window.setController(&controller);

    child<QAction>(window, "actionStartServer")->trigger();
    auto *box = window.findChild<QMessageBox *>();
    QVERIFY(box);
    QCOMPARE(box->text(), QStringLiteral("No endpoint is enabled"));
    box->close();
}

void TestMainWindow::startWarningShowsMessage()
{
    ServerController controller;
    MainWindow window;
    window.setController(&controller);

    emit controller.startWarning(QStringLiteral("Secure endpoints dropped"));
    auto *box = window.findChild<QMessageBox *>();
    QVERIFY(box);
    QCOMPARE(box->text(), QStringLiteral("Secure endpoints dropped"));
    box->close();
}

void TestMainWindow::addNodeThroughDialog()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    MainWindow window;
    window.setController(&controller);
    auto *page = child<ObjectsPage>(window, "objectsPage");
    QVERIFY(page->selectNode(QStringLiteral("ns=1;s=Tank_01.Level")));

    child<QAction>(window, "actionAddNode")->trigger();
    auto *dialog = window.findChild<AddNodeDialog *>();
    QVERIFY(dialog);
    QCOMPARE(child<QLabel>(*dialog, "parentValue")->text(), QStringLiteral("Tank_01"));
    child<QComboBox>(*dialog, "kindCombo")->setCurrentIndex(2);
    child<QLineEdit>(*dialog, "browseNameEdit")->setText(QStringLiteral("Outlet"));
    dialog->accept();

    const QString id = QStringLiteral("ns=1;s=FactoryLine_01.Tank_01.Outlet");
    QVERIFY(ProjectNodes::find(controller.project().nodes, id));
    QCOMPARE(page->currentNodeId(), id);
}

void TestMainWindow::deleteNodeAfterConfirmation()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    MainWindow window;
    window.setController(&controller);
    QVERIFY(child<ObjectsPage>(window, "objectsPage")->selectNode(QStringLiteral("ns=1;s=Pump_01")));

    child<QAction>(window, "actionDeleteNode")->trigger();
    auto *question = window.findChild<QMessageBox *>();
    QVERIFY(question);
    question->button(QMessageBox::Cancel)->click();
    QVERIFY(ProjectNodes::find(controller.project().nodes, QStringLiteral("ns=1;s=Pump_01")));

    QTRY_VERIFY(!window.findChild<QMessageBox *>());
    child<QAction>(window, "actionDeleteNode")->trigger();
    window.findChild<QMessageBox *>()->button(QMessageBox::Yes)->click();
    QVERIFY(!ProjectNodes::find(controller.project().nodes, QStringLiteral("ns=1;s=Pump_01")));
}

void TestMainWindow::deleteNodeFollowsSelection()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    MainWindow window;
    QVERIFY(!child<QAction>(window, "actionDeleteNode")->isEnabled());
    window.setController(&controller);
    auto *page = child<ObjectsPage>(window, "objectsPage");

    QVERIFY(page->selectNode(QStringLiteral("i=85")));
    QVERIFY(!child<QAction>(window, "actionDeleteNode")->isEnabled());
    QVERIFY(page->selectNode(QStringLiteral("ns=1;s=Motor_01")));
    QVERIFY(child<QAction>(window, "actionDeleteNode")->isEnabled());
    QVERIFY(child<QAction>(window, "actionRenameNode")->isEnabled());
    QVERIFY(page->selectNode(QStringLiteral("i=85")));
    QVERIFY(!child<QAction>(window, "actionRenameNode")->isEnabled());
}

void TestMainWindow::duplicatesAndPastesNodes()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    MainWindow window;
    window.setController(&controller);
    auto *page = child<ObjectsPage>(window, "objectsPage");
    auto *paste = child<QAction>(window, "actionPasteNode");
    QVERIFY(!paste->isEnabled());

    QVERIFY(page->selectNode(QStringLiteral("ns=1;s=Motor_01")));
    child<QAction>(window, "actionDuplicateNode")->trigger();
    const QString duplicate = page->currentNodeId();
    QCOMPARE(ProjectNodes::find(controller.project().nodes, duplicate)->browseName, QStringLiteral("Motor_01_copy"));
    QCOMPARE(ProjectNodes::parentId(controller.project().nodes, duplicate), QStringLiteral("ns=1;s=FactoryLine_01"));

    QVERIFY(page->selectNode(QStringLiteral("ns=1;s=Motor_01.Speed")));
    child<QAction>(window, "actionCopyNode")->trigger();
    QVERIFY(paste->isEnabled());
    QVERIFY(page->selectNode(QStringLiteral("ns=1;s=Pump_01")));
    paste->trigger();
    const QString pasted = page->currentNodeId();
    QCOMPARE(ProjectNodes::parentId(controller.project().nodes, pasted), QStringLiteral("ns=1;s=Pump_01"));
    QCOMPARE(ProjectNodes::find(controller.project().nodes, pasted)->browseName, QStringLiteral("Speed"));
}

void TestMainWindow::deletesSelectedNodes()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    MainWindow window;
    window.setController(&controller);
    auto *page = child<ObjectsPage>(window, "objectsPage");
    QVERIFY(page->selectNode(QStringLiteral("ns=1;s=Motor_01")));
    auto *tree = child<QTreeView>(window, "addressTree");
    const QModelIndexList pump = tree->model()->match(tree->model()->index(0, 0), ItemRoles::Id,
                                                      QStringLiteral("ns=1;s=Pump_01"), 1,
                                                      Qt::MatchExactly | Qt::MatchRecursive);
    QCOMPARE(pump.size(), 1);
    tree->selectionModel()->select(pump.first(), QItemSelectionModel::Select | QItemSelectionModel::Rows);
    QCOMPARE(page->selectedNodeIds().size(), 2);

    child<QAction>(window, "actionDeleteNode")->trigger();
    auto *question = window.findChild<QMessageBox *>();
    QVERIFY(question);
    QVERIFY(question->text().contains(QStringLiteral("2 nodes")));
    question->button(QMessageBox::Yes)->click();
    QVERIFY(!ProjectNodes::find(controller.project().nodes, QStringLiteral("ns=1;s=Motor_01")));
    QVERIFY(!ProjectNodes::find(controller.project().nodes, QStringLiteral("ns=1;s=Pump_01")));
    QVERIFY(ProjectNodes::find(controller.project().nodes, QStringLiteral("ns=1;s=Tank_01")));
}

void TestMainWindow::renamesNodeFromDialog()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    MainWindow window;
    window.setController(&controller);
    auto *page = child<ObjectsPage>(window, "objectsPage");
    const QString motor = QStringLiteral("ns=1;s=Motor_01");
    QVERIFY(page->selectNode(motor));

    child<QAction>(window, "actionRenameNode")->trigger();
    auto *dialog = window.findChild<QInputDialog *>();
    QVERIFY(dialog);
    QCOMPARE(dialog->textValue(), QStringLiteral("Motor_01"));
    dialog->setTextValue(QStringLiteral(" Drive "));
    dialog->accept();
    QCOMPARE(ProjectNodes::find(controller.project().nodes, motor)->browseName, QStringLiteral("Drive"));
    QCOMPARE(page->currentNodeId(), motor);

    QTRY_VERIFY(!window.findChild<QInputDialog *>());
    child<QAction>(window, "actionRenameNode")->trigger();
    dialog = window.findChild<QInputDialog *>();
    dialog->setTextValue(QStringLiteral("Pump_01"));
    dialog->accept();
    auto *box = window.findChild<QMessageBox *>();
    QVERIFY(box);
    QVERIFY(box->text().contains(QStringLiteral("Pump_01")));
    box->close();
    QCOMPARE(ProjectNodes::find(controller.project().nodes, motor)->browseName, QStringLiteral("Drive"));
}

void TestMainWindow::switchesLanguageWithoutRestart()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    MainWindow window;
    window.setController(&controller);
    window.show();
    QVERIFY(child<ObjectsPage>(window, "objectsPage")->selectNode(QStringLiteral("ns=1;s=Motor_01")));
    auto *variablesTitle = child<QLabel>(window, "variablesTitle");
    auto *tabs = child<QTabWidget>(window, "tabWidget");
    auto *running = child<StatusIndicator>(window, "serverStatusIndicator");
    QCOMPARE(variablesTitle->text(), QStringLiteral("Variables in 'Motor_01'"));

    child<QAction>(window, "actionLanguageRussian")->trigger();
    QCOMPARE(AppLanguage::saved(), AppLanguage::Language::Russian);
    QTRY_COMPARE(child<QMenu>(window, "menuFile")->title(), QStringLiteral("&Файл"));
    QTRY_COMPARE(variablesTitle->text(), QStringLiteral("Переменные «Motor_01»"));
    QCOMPARE(tabs->tabText(tabs->indexOf(child<ObjectsPage>(window, "objectsPage"))), QStringLiteral("Объекты"));
    QTRY_COMPARE(running->text(), QStringLiteral("Остановлен"));
    QVERIFY(child<QAction>(window, "actionLanguageRussian")->isChecked());

    child<QAction>(window, "actionLanguageEnglish")->trigger();
    QTRY_COMPARE(child<QMenu>(window, "menuFile")->title(), QStringLiteral("&File"));
    QTRY_COMPARE(variablesTitle->text(), QStringLiteral("Variables in 'Motor_01'"));
    QTRY_COMPARE(running->text(), QStringLiteral("Stopped"));
    QCOMPARE(AppLanguage::saved(), AppLanguage::Language::English);
}

void TestMainWindow::configuresAlarmFromDialog()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    MainWindow window;
    window.setController(&controller);
    const QString speed = QStringLiteral("ns=1;s=Motor_01.Speed");
    QVERIFY(child<ObjectsPage>(window, "objectsPage")->selectNode(speed));
    const QVariant initialValue = ProjectNodes::find(controller.project().nodes, speed)->initialValue;

    child<QPushButton>(window, "alarmButton")->click();
    auto *dialog = window.findChild<AlarmDialog *>();
    QVERIFY(dialog);
    child<QGroupBox>(*dialog, "alarmGroup")->setChecked(true);
    child<QCheckBox>(*dialog, "highCheck")->setChecked(true);
    child<QDoubleSpinBox>(*dialog, "highSpin")->setValue(1400.0);
    dialog->accept();

    const NodeDefinition *node = ProjectNodes::find(controller.project().nodes, speed);
    QVERIFY(node->alarm.enabled);
    QCOMPARE(node->alarm.high, 1400.0);
    QCOMPARE(node->initialValue, initialValue);
    QCOMPARE(node->simulation.function, SimulationFunction::Sine);
}

void TestMainWindow::appliesUsersFromDialog()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    MainWindow window;
    window.setController(&controller);

    child<QAction>(window, "actionUserAccounts")->trigger();
    auto *dialog = window.findChild<UsersDialog *>();
    QVERIFY(dialog);
    child<QPushButton>(*dialog, "addUserButton")->click();
    auto *table = child<QTableWidget>(*dialog, "usersTable");
    table->item(0, 0)->setText(QStringLiteral("operator"));
    table->item(0, 1)->setText(QStringLiteral("secret"));
    dialog->accept();

    QCOMPARE(controller.project().users.size(), 1);
    QCOMPARE(controller.project().users.first().username, QStringLiteral("operator"));
}

void TestMainWindow::appliesServerSettingsFromDialog()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    MainWindow window;
    window.setController(&controller);

    child<QAction>(window, "actionServerSettings")->trigger();
    auto *dialog = window.findChild<ServerSettingsDialog *>();
    QVERIFY(dialog);
    child<QLineEdit>(*dialog, "applicationUriEdit")->setText(QStringLiteral("urn:test:line"));
    child<QSpinBox>(*dialog, "maxSessionsSpin")->setValue(7);
    dialog->accept();

    QCOMPARE(controller.project().server.applicationUri, QStringLiteral("urn:test:line"));
    QCOMPARE(controller.project().server.maxSessions, 7);
}

void TestMainWindow::refusedVariableChangeShowsMessage()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    MainWindow window;
    window.setController(&controller);
    auto *page = child<ObjectsPage>(window, "objectsPage");
    QVERIFY(page->selectNode(QStringLiteral("ns=1;s=Motor_01.Speed")));

    child<QLineEdit>(*page, "displayNameEdit")->setText(QStringLiteral("Running"));
    child<QPushButton>(*page, "applyButton")->click();
    auto *box = window.findChild<QMessageBox *>();
    QVERIFY(box);
    QCOMPARE(box->text(), QStringLiteral("A sibling is already named 'Running'"));
    box->close();
}

void TestMainWindow::endpointChangesShowRestartNote()
{
    ProjectConfig project = DefaultProject::create();
    project.endpoints.first().port = TestPort::freePort();
    ServerController controller;
    controller.setProject(project);
    MainWindow window;
    window.setController(&controller);
    window.show();
    QVERIFY(controller.start());

    auto *endpoints = child<EndpointsPage>(window, "endpointsPage");
    const quint16 port = TestPort::freePort();
    child<QSpinBox>(*endpoints, "portSpin")->setValue(port);
    child<QPushButton>(*endpoints, "applyButton")->click();

    QCOMPARE(controller.project().endpoints.first().port, int(port));
    const auto notes = window.statusBar()->findChildren<QLabel *>();
    const auto note = std::find_if(notes.cbegin(), notes.cend(), [](const QLabel *label) {
        return label->text().startsWith(QStringLiteral("Restart the server"));
    });
    QVERIFY(note != notes.cend());
    QVERIFY((*note)->isVisible());

    child<QAction>(window, "actionRestartServer")->trigger();
    QVERIFY(!(*note)->isVisible());
    QCOMPARE(child<QLineEdit>(window, "endpointEdit")->text(), QStringLiteral("opc.tcp://localhost:%1").arg(port));
}

void TestMainWindow::refusedEndpointChangeShowsMessage()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    MainWindow window;
    window.setController(&controller);

    auto *endpoints = child<EndpointsPage>(window, "endpointsPage");
    child<QPushButton>(*endpoints, "addEndpointButton")->click();
    QCOMPARE(controller.project().endpoints.size(), 2);
    ViewSetup::selectRow(child<QTableView>(*endpoints, "endpointsView"), 1);
    child<QSpinBox>(*endpoints, "portSpin")->setValue(4999);
    child<QPushButton>(*endpoints, "applyButton")->click();

    auto *box = window.findChild<QMessageBox *>();
    QVERIFY(box);
    QVERIFY(box->text().contains(QStringLiteral("must share one port")));
    box->close();
}

void TestMainWindow::titleShowsDocumentAndChanges()
{
    ServerController controller;
    ProjectDocument document(&controller);
    document.newProject();
    MainWindow window;
    window.setController(&controller);
    window.setDocument(&document);

    QCOMPARE(window.windowTitle(), QStringLiteral("Untitled[*] - OpenUaSim"));
    QVERIFY(!window.isWindowModified());
    QVERIFY(controller.removeNode(QStringLiteral("ns=1;s=Pump_01")));
    QVERIFY(window.isWindowModified());

    QTemporaryDir directory;
    QVERIFY(window.saveProjectTo(directory.filePath(QStringLiteral("line.ouasim"))));
    QCOMPARE(window.windowTitle(), QStringLiteral("line.ouasim[*] - OpenUaSim"));
    QVERIFY(!window.isWindowModified());
}

void TestMainWindow::savesAndOpensProjectFiles()
{
    QTemporaryDir directory;
    const QString fileName = directory.filePath(QStringLiteral("line.ouasim"));
    ServerController controller;
    ProjectDocument document(&controller);
    document.newProject();
    MainWindow window;
    window.setController(&controller);
    window.setDocument(&document);

    QVERIFY(controller.removeNode(QStringLiteral("ns=1;s=Pump_01")));
    QVERIFY(window.saveProjectTo(fileName));
    document.newProject();
    QVERIFY(window.openProjectFile(fileName));
    QVERIFY(!ProjectNodes::find(controller.project().nodes, QStringLiteral("ns=1;s=Pump_01")));
    QCOMPARE(child<QTableView>(window, "variablesView")->model()->rowCount(), 3);
}

void TestMainWindow::openFailureShowsMessage()
{
    ServerController controller;
    ProjectDocument document(&controller);
    document.newProject();
    MainWindow window;
    window.setController(&controller);
    window.setDocument(&document);

    QVERIFY(!window.openProjectFile(QStringLiteral("missing.ouasim")));
    auto *box = window.findChild<QMessageBox *>();
    QVERIFY(box);
    QVERIFY(box->text().startsWith(QStringLiteral("Cannot open")));
    box->close();
}

void TestMainWindow::closingAsksAboutUnsavedChanges()
{
    ServerController controller;
    ProjectDocument document(&controller);
    document.newProject();
    MainWindow window;
    window.setController(&controller);
    window.setDocument(&document);
    window.show();
    QVERIFY(controller.removeNode(QStringLiteral("ns=1;s=Pump_01")));

    QTimer::singleShot(0, &window, [&window] {
        window.findChild<QMessageBox *>()->button(QMessageBox::Cancel)->click();
    });
    QVERIFY(!window.close());
    QVERIFY(window.isVisible());

    QTimer::singleShot(0, &window, [&window] {
        window.findChild<QMessageBox *>()->button(QMessageBox::Discard)->click();
    });
    QVERIFY(window.close());
}

void TestMainWindow::importsAndExportsModels()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    MainWindow window;
    window.setController(&controller);

    QTemporaryDir directory;
    const QString exported = directory.filePath(QStringLiteral("model.xml"));
    QVERIFY(window.exportModelTo(exported));
    QVERIFY(QFile::exists(exported));

    QVERIFY(!window.importModelFrom(directory.filePath(QStringLiteral("missing.xml"))));
    auto *box = window.findChild<QMessageBox *>();
    QVERIFY(box);
    QCOMPARE(box->icon(), QMessageBox::Warning);
    QVERIFY(!box->text().isEmpty());
#ifndef Q_OS_MACOS // QMessageBox drops the window title on macOS.
    QCOMPARE(box->windowTitle(), QStringLiteral("Model not imported"));
#endif
    box->close();
}

void TestMainWindow::removesModelAfterConfirmation()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    QVERIFY(controller.importNodeSet(QStringLiteral(OUASIM_TEST_DATA_DIR "/mixer.NodeSet2.xml")));
    MainWindow window;
    window.setController(&controller);
    auto *view = child<QTableView>(window, "namespacesView");
    ViewSetup::selectRow(view, view->model()->rowCount() - 1);

    child<QPushButton>(window, "removeModelButton")->click();
    auto *question = window.findChild<QMessageBox *>();
    QVERIFY(question);
    question->button(QMessageBox::Cancel)->click();
    QCOMPARE(controller.project().nodeSetFiles.size(), 1);

    QTRY_VERIFY(!window.findChild<QMessageBox *>());
    child<QPushButton>(window, "removeModelButton")->click();
    window.findChild<QMessageBox *>()->button(QMessageBox::Yes)->click();
    QVERIFY(controller.project().nodeSetFiles.isEmpty());
    QCOMPARE(view->model()->rowCount(), controller.snapshot().namespaces.size());
}

void TestMainWindow::exportsNamespaceAndLog()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    MainWindow window;
    window.setController(&controller);

    QTemporaryDir directory;
    const QString model = directory.filePath(QStringLiteral("namespace.xml"));
    QVERIFY(window.exportNamespaceTo(model, controller.snapshot().namespaces.value(1).uri));
    QVERIFY(QFile::exists(model));

    const QString log = directory.filePath(QStringLiteral("log.csv"));
    QVERIFY(window.exportLogTo(log, controller.log()));
    QVERIFY(QFile::exists(log));

    QVERIFY(!window.exportLogTo(directory.filePath(QStringLiteral("no/such/dir/log.csv")), controller.log()));
    auto *box = window.findChild<QMessageBox *>();
    QVERIFY(box);
    QCOMPARE(box->icon(), QMessageBox::Warning);
#ifndef Q_OS_MACOS // QMessageBox drops the window title on macOS.
    QCOMPARE(box->windowTitle(), QStringLiteral("Log not exported"));
#endif
    box->close();
}

void TestMainWindow::panelsFitTheDefaultWindow_data()
{
    QTest::addColumn<int>("language");
    QTest::newRow("English") << int(AppLanguage::Language::English);
    QTest::newRow("Russian") << int(AppLanguage::Language::Russian);
}

void TestMainWindow::panelsFitTheDefaultWindow()
{
    QFETCH(int, language);
    AppLanguage::apply(*QCoreApplication::instance(), AppLanguage::Language(language));
    QCoreApplication::processEvents();
    const QFont windowsFont(QStringLiteral("Segoe UI"), 9);
    if (!QFontDatabase::families().contains(windowsFont.family()))
        QSKIP("Needs the Segoe UI font the default window size was designed for");

    // Last in the suite: the application style and font change for every widget.
    const QString previousStyle = QApplication::style()->name();
    const QFont previousFont = QApplication::font();
    QApplication::setStyle(new AppStyle);
    QApplication::setFont(windowsFont);

    QStringList overflowing;
    {
        MainWindow window;
        window.setSnapshot(MockData::snapshot());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto *tabs = child<QTabWidget>(window, "tabWidget");
        for (int i = 0; i < tabs->count(); ++i) {
            tabs->setCurrentIndex(i);
            QApplication::processEvents();
            const QList<QScrollArea *> bodies = tabs->widget(i)->findChildren<QScrollArea *>();
            for (const QScrollArea *body : bodies) {
                if (!body->property(AppStyle::panelBodyProperty).toBool())
                    continue;
                const QSize needed = body->widget()->minimumSizeHint();
                const QSize shown = body->viewport()->size();
                if (needed.height() > shown.height() || needed.width() > shown.width())
                    overflowing.append(QStringLiteral("%1/%2: needs %3x%4, shows %5x%6")
                                           .arg(tabs->tabText(i), body->objectName())
                                           .arg(needed.width()).arg(needed.height())
                                           .arg(shown.width()).arg(shown.height()));
            }
        }
    }

    QApplication::setFont(previousFont);
    QApplication::setStyle(previousStyle);
    AppLanguage::apply(*QCoreApplication::instance(), AppLanguage::Language::English);
    QVERIFY2(overflowing.isEmpty(), qPrintable(overflowing.join(QStringLiteral("; "))));
}

QTEST_MAIN(TestMainWindow)
#include "test_mainwindow.moc"
