// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QTableView>
#include <QTest>
#include <QTreeView>

#include "certificatesmodel.h"
#include "certificatespage.h"
#include "endpointsmodel.h"
#include "endpointspage.h"
#include "logfilterproxymodel.h"
#include "logmodel.h"
#include "logpage.h"
#include "mock/mockdata.h"
#include "namespacesmodel.h"
#include "namespacespage.h"
#include "objectspage.h"
#include "sessionsmodel.h"
#include "sessionspage.h"
#include "statusindicator.h"
#include "statuspage.h"
#include "subtypesmodel.h"
#include "typespage.h"
#include "variablesmodel.h"
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

QString text(const QWidget &parent, const char *name)
{
    return child<QLineEdit>(parent, name)->text();
}

}

class TestPages : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void statusPage();
    void objectsPageStartsOnCurrentVariable();
    void objectsPageFollowsVariableSelection();
    void objectsPageShowsObjects();
    void objectsPageClearsForFolders();
    void typesPage();
    void namespacesPage();
    void endpointsPage();
    void certificatesPage();
    void sessionsPage();
    void sessionsPageFiltersByState();
    void logPageFilters();
    void logPageSelectsFirstEntryWhenFilledLater();
    void logPageDetailsAndClear();

private:
    ServerSnapshot _snapshot;
};

void TestPages::initTestCase()
{
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
    _snapshot = MockData::snapshot();
}

void TestPages::statusPage()
{
    StatusPage page;
    page.setStatus(_snapshot.status);

    QCOMPARE(child<StatusIndicator>(page, "serverStatusIndicator")->status(), StatusKind::Good);
    QCOMPARE(child<StatusIndicator>(page, "httpsIndicator")->status(), StatusKind::Inactive);
    QCOMPARE(child<QLabel>(page, "primaryEndpointValue")->text(), _snapshot.status.primaryEndpoint);
    QCOMPARE(child<QLabel>(page, "uptimeValue")->text(), QStringLiteral("00:27:36"));
    QCOMPARE(child<QLabel>(page, "totalNodesValue")->text(), QString::number(_snapshot.status.totalNodes));
    QCOMPARE(child<QLabel>(page, "clientsValue")->text(), QStringLiteral("1"));
    QCOMPARE(child<QLabel>(page, "securityModesValue")->text(), QStringLiteral("None, Sign, Sign & Encrypt"));
    QVERIFY(!page.findChild<QWidget *>(QStringLiteral("statisticsPanel")));
}

void TestPages::objectsPageStartsOnCurrentVariable()
{
    ObjectsPage page;
    page.setAddressSpace(_snapshot.addressSpace, _snapshot.variables);

    QCOMPARE(page.variablesModel()->rowCount(), 3);
    QCOMPARE(child<QLabel>(page, "variablesTitle")->text(), QStringLiteral("Variables in 'Motor_01'"));
    QCOMPARE(child<QTableView>(page, "variablesView")->currentIndex().row(), 0);
    QCOMPARE(text(page, "nodeIdEdit"), QStringLiteral("ns=2;s=Motor_01.Speed"));
    QCOMPARE(child<QComboBox>(page, "dataTypeCombo")->currentText(), QStringLiteral("Double"));
    QCOMPARE(child<QComboBox>(page, "simulationCombo")->currentText(), QStringLiteral("Sine wave"));
    QCOMPARE(child<QDoubleSpinBox>(page, "amplitudeSpin")->value(), 500.0);
    QVERIFY(child<QWidget>(page, "propertiesHost")->isEnabled());
}

void TestPages::objectsPageFollowsVariableSelection()
{
    ObjectsPage page;
    page.setAddressSpace(_snapshot.addressSpace, _snapshot.variables);

    ViewSetup::selectRow(child<QTableView>(page, "variablesView"), 2);
    QCOMPARE(text(page, "nodeIdEdit"), QStringLiteral("ns=2;s=Motor_01.Running"));
    QCOMPARE(child<QComboBox>(page, "dataTypeCombo")->currentText(), QStringLiteral("Boolean"));
    QCOMPARE(child<QComboBox>(page, "simulationCombo")->currentText(), QStringLiteral("None"));

    auto *tree = child<QTreeView>(page, "addressTree");
    QVERIFY(ViewSetup::selectById(tree, QStringLiteral("ns=2;s=Tank_01.Level")));
    QCOMPARE(child<QLabel>(page, "variablesTitle")->text(), QStringLiteral("Variables in 'Tank_01'"));
    QCOMPARE(text(page, "nodeIdEdit"), QStringLiteral("ns=2;s=Tank_01.Level"));
}

void TestPages::objectsPageShowsObjects()
{
    ObjectsPage page;
    page.setAddressSpace(_snapshot.addressSpace, _snapshot.variables);

    QVERIFY(ViewSetup::selectById(child<QTreeView>(page, "addressTree"), QStringLiteral("ns=2;s=Pump_01")));
    QCOMPARE(page.variablesModel()->rowCount(), 3);
    QCOMPARE(page.variablesModel()->item(0).nodeId, QStringLiteral("ns=2;s=Pump_01.Flow"));
    QCOMPARE(text(page, "nodeIdEdit"), QStringLiteral("ns=2;s=Pump_01.Flow"));
}

void TestPages::objectsPageClearsForFolders()
{
    ObjectsPage page;
    page.setAddressSpace(_snapshot.addressSpace, _snapshot.variables);

    QVERIFY(ViewSetup::selectById(child<QTreeView>(page, "addressTree"), QStringLiteral("i=85")));
    QCOMPARE(page.variablesModel()->rowCount(), 0);
    QCOMPARE(child<QLabel>(page, "variablesTitle")->text(), QStringLiteral("Variables"));
    QVERIFY(text(page, "nodeIdEdit").isEmpty());
    QVERIFY(!child<QWidget>(page, "propertiesHost")->isEnabled());
}

void TestPages::typesPage()
{
    TypesPage page;
    page.setTypes(_snapshot.typeTree, _snapshot.types);

    QCOMPARE(page.subtypesModel()->rowCount(), 3);
    QCOMPARE(child<QLabel>(page, "subtypesTitle")->text(), QStringLiteral("Subtype Definitions (DeviceType)"));
    QCOMPARE(text(page, "browseNameEdit"), QStringLiteral("DeviceType"));
    QCOMPARE(child<QComboBox>(page, "baseTypeCombo")->currentText(), QStringLiteral("BaseObjectType"));
    QCOMPARE(child<QComboBox>(page, "isAbstractCombo")->currentText(), QStringLiteral("false"));
    QCOMPARE(child<QTableView>(page, "referencesView")->model()->rowCount(), 4);

    ViewSetup::selectRow(child<QTableView>(page, "subtypesView"), 0);
    QCOMPARE(text(page, "browseNameEdit"), QStringLiteral("MotorType"));
    QCOMPARE(child<QComboBox>(page, "baseTypeCombo")->currentText(), QStringLiteral("DeviceType"));

    QVERIFY(ViewSetup::selectById(child<QTreeView>(page, "typesTree"), QStringLiteral("i=58")));
    QCOMPARE(child<QComboBox>(page, "isAbstractCombo")->currentText(), QStringLiteral("true"));
    QCOMPARE(page.subtypesModel()->rowCount(), 2);

    auto *tree = child<QTreeView>(page, "typesTree");
    tree->setCurrentIndex(tree->model()->index(0, 0));
    QCOMPARE(page.subtypesModel()->rowCount(), 0);
    QVERIFY(!child<QWidget>(page, "propertiesHost")->isEnabled());
}

void TestPages::namespacesPage()
{
    NamespacesPage page;
    page.setNamespaces(_snapshot.namespaces);

    QCOMPARE(page.namespacesModel()->rowCount(), _snapshot.namespaces.size());
    QCOMPARE(text(page, "indexEdit"), QStringLiteral("0"));
    QCOMPARE(text(page, "uriEdit"), QStringLiteral("http://opcfoundation.org/UA/"));

    child<QLineEdit>(page, "searchEdit")->setText(QStringLiteral("openuasim:types"));
    auto *view = child<QTableView>(page, "namespacesView");
    QCOMPARE(view->model()->rowCount(), 1);
    ViewSetup::selectRow(view, 0);
    QCOMPARE(text(page, "indexEdit"), QStringLiteral("4"));
    QCOMPARE(text(page, "versionEdit"), QStringLiteral("1.0.0"));
    QVERIFY(!page.findChild<QTreeView *>());
}

void TestPages::endpointsPage()
{
    EndpointsPage page;
    page.setEndpoints(_snapshot.endpoints);

    QCOMPARE(page.endpointsModel()->rowCount(), _snapshot.endpoints.size());
    QCOMPARE(text(page, "nameEdit"), QStringLiteral("OPC UA (localhost)"));
    QCOMPARE(child<QComboBox>(page, "securityModeCombo")->currentText(), QStringLiteral("Sign & Encrypt"));
    QVERIFY(!child<QLineEdit>(page, "usernameEdit")->isEnabled());

    ViewSetup::selectRow(child<QTableView>(page, "endpointsView"), 3);
    QCOMPARE(text(page, "nameEdit"), QStringLiteral("Discovery Endpoint"));
    QCOMPARE(child<QComboBox>(page, "securityPolicyCombo")->currentText(), QStringLiteral("None"));

    child<QLineEdit>(page, "searchEdit")->setText(QStringLiteral("192.168"));
    ViewSetup::selectRow(child<QTableView>(page, "endpointsView"), 0);
    QCOMPARE(text(page, "nameEdit"), QStringLiteral("OPC UA (LAN)"));

    child<QCheckBox>(page, "usernamePasswordCheck")->setChecked(true);
    QVERIFY(child<QLineEdit>(page, "usernameEdit")->isEnabled());
}

void TestPages::certificatesPage()
{
    CertificatesPage page;
    page.setCertificates(_snapshot.certificates);

    QCOMPARE(page.certificatesModel()->rowCount(), 3);
    QCOMPARE(child<QLabel>(page, "certificatesTitle")->text(), QStringLiteral("Trusted Clients"));
    QCOMPARE(text(page, "commonNameEdit"), QStringLiteral("OPC UA Client A"));
    QCOMPARE(child<StatusIndicator>(page, "statusIndicator")->status(), StatusKind::Good);
    QVERIFY(!child<QPushButton>(page, "trustButton")->isEnabled());
    QVERIFY(child<QPushButton>(page, "rejectButton")->isEnabled());

    const QString rejected = QString::number(static_cast<int>(CertificateInfo::Category::Rejected));
    QVERIFY(ViewSetup::selectById(child<QTreeView>(page, "categoriesList"), rejected));
    QCOMPARE(page.certificatesModel()->rowCount(), 1);
    QCOMPARE(child<StatusIndicator>(page, "statusIndicator")->status(), StatusKind::Error);
    QVERIFY(child<QPushButton>(page, "trustButton")->isEnabled());
    QVERIFY(!child<QPushButton>(page, "rejectButton")->isEnabled());

    const QString own = QString::number(static_cast<int>(CertificateInfo::Category::Application));
    QVERIFY(ViewSetup::selectById(child<QTreeView>(page, "categoriesList"), own));
    QVERIFY(!child<QPushButton>(page, "trustButton")->isEnabled());
    QVERIFY(!child<QPushButton>(page, "rejectButton")->isEnabled());

    page.setCertificates({});
    QCOMPARE(page.certificatesModel()->rowCount(), 0);
    QVERIFY(!child<QWidget>(page, "detailsHost")->isEnabled());
}

void TestPages::sessionsPage()
{
    SessionsPage page;
    page.setSessions(_snapshot.sessions);

    QCOMPARE(page.sessionsModel()->rowCount(), _snapshot.sessions.size());
    QCOMPARE(text(page, "sessionIdEdit"), QStringLiteral("ns=1;i=12345"));
    QCOMPARE(child<StatusIndicator>(page, "stateIndicator")->status(), StatusKind::Good);
    QCOMPARE(text(page, "subscriptionsEdit"), QStringLiteral("3"));

    auto *view = child<QTableView>(page, "sessionsView");
    ViewSetup::selectRow(view, 1);
    QCOMPARE(text(page, "sessionIdEdit"), QStringLiteral("ns=1;i=12344"));
    QCOMPARE(child<StatusIndicator>(page, "stateIndicator")->status(), StatusKind::Inactive);
    QCOMPARE(child<QComboBox>(page, "userIdentityCombo")->currentText(), QStringLiteral("Username"));
}

void TestPages::sessionsPageFiltersByState()
{
    SessionsPage page;
    page.setSessions(_snapshot.sessions);
    auto *view = child<QTableView>(page, "sessionsView");
    auto *state = child<QComboBox>(page, "stateFilterCombo");

    state->setCurrentIndex(1);
    QCOMPARE(view->model()->rowCount(), 1);
    QCOMPARE(text(page, "sessionIdEdit"), QStringLiteral("ns=1;i=12345"));

    state->setCurrentIndex(2);
    QCOMPARE(view->model()->rowCount(), _snapshot.sessions.size() - 1);
    QCOMPARE(child<StatusIndicator>(page, "stateIndicator")->status(), StatusKind::Inactive);

    child<QLineEdit>(page, "searchEdit")->setText(QStringLiteral("prosys"));
    QCOMPARE(view->model()->rowCount(), 1);
    ViewSetup::selectRow(view, 0);
    QCOMPARE(text(page, "sessionNameEdit"), QStringLiteral("Prosys Client"));

    child<QLineEdit>(page, "searchEdit")->clear();
    state->setCurrentIndex(0);
    QCOMPARE(view->model()->rowCount(), _snapshot.sessions.size());
}

void TestPages::logPageFilters()
{
    LogModel model;
    model.setItems(_snapshot.log);
    LogPage page;
    page.setLogModel(&model);
    const int total = model.rowCount();
    QCOMPARE(page.filterModel()->rowCount(), total);

    child<QCheckBox>(page, "infoCheck")->setChecked(false);
    QCOMPARE(page.filterModel()->rowCount(), 2);
    child<QCheckBox>(page, "securityCheck")->setChecked(false);
    QCOMPARE(page.filterModel()->rowCount(), 1);

    child<QCheckBox>(page, "infoCheck")->setChecked(true);
    child<QCheckBox>(page, "securityCheck")->setChecked(true);
    QCOMPARE(page.filterModel()->rowCount(), total);
}

void TestPages::logPageSelectsFirstEntryWhenFilledLater()
{
    LogModel model;
    LogPage page;
    page.setLogModel(&model);
    QVERIFY(text(page, "timeEdit").isEmpty());

    model.setItems(_snapshot.log);
    QCOMPARE(child<QTableView>(page, "logView")->currentIndex().row(), 0);
    QCOMPARE(text(page, "eventIdEdit"), QStringLiteral("1000"));
}

void TestPages::logPageDetailsAndClear()
{
    LogModel model;
    model.setItems(_snapshot.log);
    LogPage page;
    page.setLogModel(&model);

    QCOMPARE(text(page, "timeEdit"), QStringLiteral("2024-05-23 14:31:20.004"));
    QCOMPARE(text(page, "eventIdEdit"), QStringLiteral("1000"));
    QCOMPARE(child<StatusIndicator>(page, "levelIndicator")->text(), QStringLiteral("INFO"));

    child<QPushButton>(page, "clearLogButton")->click();
    QCOMPARE(model.rowCount(), 0);
    QVERIFY(text(page, "timeEdit").isEmpty());
    QVERIFY(!child<QWidget>(page, "detailsHost")->isEnabled());
}

QTEST_MAIN(TestPages)
#include "test_pages.moc"
