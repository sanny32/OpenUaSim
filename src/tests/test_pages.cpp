// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalSpy>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QTableView>
#include <QToolButton>
#include <QTest>
#include <QTreeView>

#include "certificatesmodel.h"
#include "certificatespage.h"
#include "endpointsmodel.h"
#include "endpointspage.h"
#include "itemroles.h"
#include "logfilterproxymodel.h"
#include "logmodel.h"
#include "logpage.h"
#include "mock/mockdata.h"
#include "namespacesmodel.h"
#include "namespacespage.h"
#include "objectspage.h"
#include "pki/certificategenerator.h"
#include "sessionsmodel.h"
#include "sessionspage.h"
#include "statusindicator.h"
#include "statuspage.h"
#include "subtypesmodel.h"
#include "typespage.h"
#include "variablesmodel.h"
#include "viewsetup.h"

Q_DECLARE_METATYPE(VariableInfo)
Q_DECLARE_METATYPE(CertificateInfo::Category)

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
    void objectsPageUpdatesValues();
    void objectsPageAppliesEditedProperties();
    void objectsPageRevertsEdits();
    void objectsPageKeepsSelectionOnReload();
    void objectsPageUpdatesOneVariable();
    void objectsPageUpdatesVariableIcon();
    void objectsPageKeepsUnlistedIcon();
    void objectsPageValueColumnOnlyGrows();
    void objectsPageReportsCurrentNode();
    void typesPage();
    void namespacesPage();
    void endpointsPage();
    void endpointsPageAppliesEdits();
    void endpointsPageAddsAndRemoves();
    void endpointsPageRevertsAndKeepsRow();
    void endpointsPageOffersSecurityOfTheBuild();
    void endpointsPageKeepsPolicyAndModePaired();
    void endpointsPageShowsStoredMismatch();
    void certificatesPage();
    void certificatesPageRequestsActions();
    void certificatesPageGeneratesOnlyInOwnFolder();
    void certificateDetailsShowEveryLine();
    void certificateDetailsKeepTheirWidth();
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

void TestPages::objectsPageUpdatesValues()
{
    ObjectsPage page;
    page.setAddressSpace(_snapshot.addressSpace, _snapshot.variables);

    page.updateValues({ { QStringLiteral("ns=2;s=Motor_01.Speed"), 900.0 },
                        { QStringLiteral("ns=2;s=Pump_01.Flow"), 3.0 } });
    QCOMPARE(page.variablesModel()->item(0).value.toDouble(), 900.0);

    QVERIFY(ViewSetup::selectById(child<QTreeView>(page, "addressTree"), QStringLiteral("ns=2;s=Pump_01")));
    QCOMPARE(page.variablesModel()->item(0).value.toDouble(), 3.0);
}

void TestPages::objectsPageAppliesEditedProperties()
{
    ObjectsPage page;
    page.setAddressSpace(_snapshot.addressSpace, _snapshot.variables);
    QSignalSpy requested(&page, &ObjectsPage::variableChangeRequested);

    auto *icons = child<QComboBox>(page, "iconCombo");
    QCOMPARE(icons->currentData().toString(), QStringLiteral("signal"));
    child<QLineEdit>(page, "displayNameEdit")->setText(QStringLiteral(" Rpm "));
    icons->setCurrentIndex(icons->findData(QStringLiteral("zap")));
    child<QComboBox>(page, "dataTypeCombo")->setCurrentText(QStringLiteral("Int32"));
    child<QComboBox>(page, "accessCombo")->setCurrentText(QStringLiteral("Read"));
    child<QComboBox>(page, "simulationCombo")->setCurrentText(QStringLiteral("Ramp"));
    child<QSpinBox>(page, "intervalSpin")->setValue(2);
    child<QComboBox>(page, "intervalUnitCombo")->setCurrentIndex(1);
    child<QSpinBox>(page, "periodSpin")->setValue(4000);
    child<QDoubleSpinBox>(page, "amplitudeSpin")->setValue(5.0);
    child<QDoubleSpinBox>(page, "offsetSpin")->setValue(-1.0);
    child<QPushButton>(page, "applyButton")->click();

    QCOMPARE(requested.count(), 1);
    const VariableInfo variable = requested.first().first().value<VariableInfo>();
    QCOMPARE(variable.nodeId, QStringLiteral("ns=2;s=Motor_01.Speed"));
    QCOMPARE(variable.displayName, QStringLiteral("Rpm"));
    QCOMPARE(variable.icon, QStringLiteral("zap"));
    QCOMPARE(variable.dataType, QStringLiteral("Int32"));
    QCOMPARE(variable.access, QStringLiteral("Read"));
    QCOMPARE(variable.simulation.function, SimulationFunction::Ramp);
    QCOMPARE(variable.simulation.intervalMs, 2000);
    QCOMPARE(variable.simulation.periodMs, 4000);
    QCOMPARE(variable.simulation.amplitude, 5.0);
    QCOMPARE(variable.simulation.offset, -1.0);
}

void TestPages::objectsPageRevertsEdits()
{
    ObjectsPage page;
    page.setAddressSpace(_snapshot.addressSpace, _snapshot.variables);
    child<QLineEdit>(page, "displayNameEdit")->setText(QStringLiteral("Changed"));
    child<QPushButton>(page, "revertButton")->click();
    QCOMPARE(text(page, "displayNameEdit"), QStringLiteral("Speed"));
}

void TestPages::objectsPageKeepsSelectionOnReload()
{
    ObjectsPage page;
    page.setAddressSpace(_snapshot.addressSpace, _snapshot.variables);
    QVERIFY(page.selectNode(QStringLiteral("ns=2;s=Tank_01.Level")));
    ViewSetup::selectRow(child<QTableView>(page, "variablesView"), 1);

    page.setAddressSpace(_snapshot.addressSpace, _snapshot.variables);
    QCOMPARE(page.currentNodeId(), QStringLiteral("ns=2;s=Tank_01.Temperature"));
    QCOMPARE(page.currentContainerId(), QStringLiteral("ns=2;s=Tank_01"));
}

void TestPages::objectsPageUpdatesOneVariable()
{
    ObjectsPage page;
    page.setAddressSpace(_snapshot.addressSpace, _snapshot.variables);
    VariableInfo speed = page.variablesModel()->item(0);
    speed.access = QStringLiteral("Read");
    speed.simulation.amplitude = 7.0;
    page.updateVariable(speed);

    QCOMPARE(page.variablesModel()->item(0).access, QStringLiteral("Read"));
    QCOMPARE(child<QDoubleSpinBox>(page, "amplitudeSpin")->value(), 7.0);
    QCOMPARE(child<QComboBox>(page, "accessCombo")->currentText(), QStringLiteral("Read"));
}

void TestPages::objectsPageUpdatesVariableIcon()
{
    ObjectsPage page;
    page.setAddressSpace(_snapshot.addressSpace, _snapshot.variables);
    VariableInfo speed = page.variablesModel()->item(0);
    speed.icon = QStringLiteral("gauge");
    page.updateVariable(speed);

    QCOMPARE(child<QComboBox>(page, "iconCombo")->currentData().toString(), QStringLiteral("gauge"));
    QCOMPARE(page.variablesModel()->data(page.variablesModel()->index(0, VariablesModel::ColName),
                                         ItemRoles::IconName).toString(),
             QStringLiteral("gauge"));
    const QAbstractItemModel *tree = child<QTreeView>(page, "addressTree")->model();
    const QModelIndexList found = tree->match(tree->index(0, 0), ItemRoles::Id, speed.nodeId, 1,
                                              Qt::MatchExactly | Qt::MatchRecursive);
    QCOMPARE(found.size(), 1);
    QCOMPARE(found.first().data(ItemRoles::IconName).toString(), QStringLiteral("gauge"));
}

void TestPages::objectsPageKeepsUnlistedIcon()
{
    ObjectsPage page;
    page.setAddressSpace(_snapshot.addressSpace, _snapshot.variables);
    VariableInfo speed = page.variablesModel()->item(0);
    speed.icon = QStringLiteral("custom-icon");
    page.updateVariable(speed);
    QCOMPARE(child<QComboBox>(page, "iconCombo")->currentIndex(), -1);

    QSignalSpy requested(&page, &ObjectsPage::variableChangeRequested);
    child<QPushButton>(page, "applyButton")->click();
    QCOMPARE(requested.count(), 1);
    QCOMPARE(requested.first().first().value<VariableInfo>().icon, QStringLiteral("custom-icon"));
}

void TestPages::objectsPageValueColumnOnlyGrows()
{
    ObjectsPage page;
    page.setAddressSpace(_snapshot.addressSpace, _snapshot.variables);
    const QHeaderView *header = child<QTableView>(page, "variablesView")->horizontalHeader();
    const QString speed = QStringLiteral("ns=2;s=Motor_01.Speed");

    page.updateValues({ { speed, 1.0 } });
    const int initial = header->sectionSize(VariablesModel::ColValue);
    page.updateValues({ { speed, 1234567.891234 } });
    const int grown = header->sectionSize(VariablesModel::ColValue);
    QVERIFY(grown > initial);
    page.updateValues({ { speed, 2.0 } });
    QCOMPARE(header->sectionSize(VariablesModel::ColValue), grown);
}

void TestPages::objectsPageReportsCurrentNode()
{
    ObjectsPage page;
    page.setAddressSpace(_snapshot.addressSpace, _snapshot.variables);
    QSignalSpy changed(&page, &ObjectsPage::currentNodeChanged);
    QVERIFY(page.selectNode(QStringLiteral("i=85")));
    QCOMPARE(page.currentNodeId(), QStringLiteral("i=85"));
    QVERIFY(page.currentContainerId() == QStringLiteral("i=85"));
    QVERIFY(changed.count() > 0);
    QCOMPARE(changed.last().first().toString(), QStringLiteral("i=85"));
    QVERIFY(!page.selectNode(QStringLiteral("ns=9;s=None")));
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

void TestPages::endpointsPageAppliesEdits()
{
    QList<EndpointInfo> endpoints = _snapshot.endpoints;
    endpoints[0].password = QStringLiteral("old");
    EndpointsPage page;
    page.setEndpoints(endpoints);
    QSignalSpy requested(&page, &EndpointsPage::endpointsChangeRequested);
    QVERIFY(child<QLineEdit>(page, "passwordEdit")->text().isEmpty());
    QCOMPARE(child<QLineEdit>(page, "passwordEdit")->placeholderText(), QStringLiteral("Unchanged"));

    child<QLineEdit>(page, "nameEdit")->setText(QStringLiteral("Edited"));
    child<QSpinBox>(page, "portSpin")->setValue(4900);
    child<QCheckBox>(page, "usernamePasswordCheck")->setChecked(true);
    child<QLineEdit>(page, "usernameEdit")->setText(QStringLiteral("operator"));
    child<QPushButton>(page, "applyButton")->click();

    QCOMPARE(requested.count(), 1);
    using Endpoints = QList<EndpointInfo>;
    const Endpoints applied = requested.first().first().value<Endpoints>();
    QCOMPARE(applied.size(), endpoints.size());
    QCOMPARE(applied[0].name, QStringLiteral("Edited"));
    QCOMPARE(applied[0].port, 4900);
    QVERIFY(applied[0].usernamePassword);
    QCOMPARE(applied[0].username, QStringLiteral("operator"));
    QCOMPARE(applied[0].password, QStringLiteral("old"));

    child<QLineEdit>(page, "passwordEdit")->setText(QStringLiteral("new"));
    child<QPushButton>(page, "applyButton")->click();
    QCOMPARE(requested.last().first().value<Endpoints>()[0].password, QStringLiteral("new"));
}

void TestPages::endpointsPageAddsAndRemoves()
{
    EndpointsPage page;
    page.setEndpoints(_snapshot.endpoints);
    QSignalSpy requested(&page, &EndpointsPage::endpointsChangeRequested);
    using Endpoints = QList<EndpointInfo>;

    child<QPushButton>(page, "addEndpointButton")->click();
    const Endpoints added = requested.last().first().value<Endpoints>();
    QCOMPARE(added.size(), _snapshot.endpoints.size() + 1);
    QCOMPARE(added.last().name, QStringLiteral("New endpoint"));
    QCOMPARE(added.last().port, _snapshot.endpoints.first().port);
    QCOMPARE(added.last().securityMode, QStringLiteral("None"));

    ViewSetup::selectRow(child<QTableView>(page, "endpointsView"), 1);
    child<QPushButton>(page, "removeEndpointButton")->click();
    const Endpoints removed = requested.last().first().value<Endpoints>();
    QCOMPARE(removed.size(), _snapshot.endpoints.size() - 1);
    QCOMPARE(removed[1].name, _snapshot.endpoints[2].name);

    page.setEndpoints({});
    QVERIFY(!child<QPushButton>(page, "removeEndpointButton")->isEnabled());
}

void TestPages::endpointsPageRevertsAndKeepsRow()
{
    EndpointsPage page;
    page.setEndpoints(_snapshot.endpoints);
    ViewSetup::selectRow(child<QTableView>(page, "endpointsView"), 2);
    child<QLineEdit>(page, "nameEdit")->setText(QStringLiteral("Changed"));
    child<QPushButton>(page, "revertButton")->click();
    QCOMPARE(text(page, "nameEdit"), _snapshot.endpoints[2].name);

    page.setEndpoints(_snapshot.endpoints);
    QCOMPARE(text(page, "nameEdit"), _snapshot.endpoints[2].name);
}

void TestPages::endpointsPageOffersSecurityOfTheBuild()
{
    EndpointsPage page;
    const auto *policies = qobject_cast<QStandardItemModel *>(child<QComboBox>(page, "securityPolicyCombo")->model());
    QVERIFY(policies);
    QVERIFY(policies->item(0)->isEnabled());
#ifdef OUASIM_HAVE_ENCRYPTION
    QVERIFY(policies->item(3)->isEnabled());
#else
    QVERIFY(!policies->item(3)->isEnabled());
    QVERIFY(!policies->item(3)->toolTip().isEmpty());
#endif
}

void TestPages::certificatesPageRequestsActions()
{
    CertificatesPage page;
    page.setCertificates(_snapshot.certificates);
    QSignalSpy imported(&page, &CertificatesPage::importRequested);
    QSignalSpy moved(&page, &CertificatesPage::moveRequested);
    QSignalSpy deleted(&page, &CertificatesPage::deleteRequested);
    QSignalSpy exported(&page, &CertificatesPage::exportRequested);
    const QString selected = page.certificatesModel()->item(0).filePath;

    child<QPushButton>(page, "importButton")->click();
    QCOMPARE(imported.first().first().value<CertificateInfo::Category>(), CertificateInfo::Category::Trusted);
    child<QPushButton>(page, "rejectButton")->click();
    QCOMPARE(moved.first().at(0).toString(), selected);
    QCOMPARE(moved.first().at(1).value<CertificateInfo::Category>(), CertificateInfo::Category::Rejected);
    child<QToolButton>(page, "deleteButton")->click();
    QCOMPARE(deleted.first().first().toString(), selected);
    child<QPushButton>(page, "exportButton")->click();
    QCOMPARE(exported.first().first().toString(), selected);

    const QString rejected = QString::number(static_cast<int>(CertificateInfo::Category::Rejected));
    QVERIFY(ViewSetup::selectById(child<QTreeView>(page, "categoriesList"), rejected));
    page.setCertificates(_snapshot.certificates);
    QCOMPARE(child<QLabel>(page, "certificatesTitle")->text(), QStringLiteral("Rejected Certificates"));
}

void TestPages::endpointsPageKeepsPolicyAndModePaired()
{
    EndpointsPage page;
    page.setEndpoints(_snapshot.endpoints);
    auto *policy = child<QComboBox>(page, "securityPolicyCombo");
    auto *mode = child<QComboBox>(page, "securityModeCombo");

    policy->setCurrentText(QStringLiteral("None"));
    QCOMPARE(mode->currentText(), QStringLiteral("None"));

    policy->setCurrentText(QStringLiteral("Aes256_Sha256_RsaPss"));
    QCOMPARE(mode->currentText(), QStringLiteral("Sign & Encrypt"));

    mode->setCurrentText(QStringLiteral("Sign"));
    QCOMPARE(policy->currentText(), QStringLiteral("Aes256_Sha256_RsaPss"));

    mode->setCurrentText(QStringLiteral("None"));
    QCOMPARE(policy->currentText(), QStringLiteral("None"));

    mode->setCurrentText(QStringLiteral("Sign"));
    QCOMPARE(policy->currentText(), QStringLiteral("Basic256Sha256"));
}

void TestPages::endpointsPageShowsStoredMismatch()
{
    QList<EndpointInfo> endpoints = _snapshot.endpoints;
    endpoints[0].securityPolicy = QStringLiteral("Aes256_Sha256_RsaPss");
    endpoints[0].securityMode = QStringLiteral("None");
    EndpointsPage page;
    page.setEndpoints(endpoints);
    QCOMPARE(child<QComboBox>(page, "securityPolicyCombo")->currentText(), QStringLiteral("Aes256_Sha256_RsaPss"));
    QCOMPARE(child<QComboBox>(page, "securityModeCombo")->currentText(), QStringLiteral("None"));
}

void TestPages::certificatesPage()
{
    CertificatesPage page;
    page.setCertificates(_snapshot.certificates);

    QCOMPARE(page.certificatesModel()->rowCount(), 3);
    const QHeaderView *columns = child<QTableView>(page, "certificatesView")->horizontalHeader();
    QCOMPARE(columns->sectionResizeMode(CertificatesModel::ColCommonName), QHeaderView::Interactive);
    QVERIFY(columns->sectionSize(CertificatesModel::ColCommonName) != columns->defaultSectionSize());
    QCOMPARE(child<QLabel>(page, "certificatesTitle")->text(), QStringLiteral("Trusted Clients"));
    QVERIFY(child<QPlainTextEdit>(page, "subjectEdit")->toPlainText().startsWith(QStringLiteral("CN=OPC UA Client A")));
    QCOMPARE(text(page, "validFromEdit"), QStringLiteral("2024-04-10 10:00:00"));
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

void TestPages::certificatesPageGeneratesOnlyInOwnFolder()
{
    CertificatesPage page;
    page.setCertificates(_snapshot.certificates);
    page.show();
    auto *generate = child<QPushButton>(page, "generateButton");
    auto *categories = child<QTreeView>(page, "categoriesList");
    QSignalSpy requested(&page, &CertificatesPage::generateRequested);

    QVERIFY(ViewSetup::selectById(categories, QString::number(int(CertificateInfo::Category::Trusted))));
    QVERIFY(!generate->isVisible());
    QVERIFY(ViewSetup::selectById(categories, QString::number(int(CertificateInfo::Category::Application))));
    QVERIFY(generate->isVisible());
    QCOMPARE(generate->isEnabled(), CertificateGenerator::isAvailable());
    generate->click();
    QCOMPARE(requested.count(), CertificateGenerator::isAvailable() ? 1 : 0);
}

void TestPages::certificateDetailsShowEveryLine()
{
    CertificatesPage page;
    page.resize(1300, 600);
    page.setCertificates(_snapshot.certificates);
    page.show();
    QVERIFY(QTest::qWaitForWindowExposed(&page));

    for (const char *name : { "subjectEdit", "issuerEdit" }) {
        auto *edit = child<QPlainTextEdit>(page, name);
        QCOMPARE(edit->blockCount(), 3);
        QVERIFY2(edit->verticalScrollBar()->maximum() == 0, name);
    }
    const auto *subject = child<QPlainTextEdit>(page, "subjectEdit");
    const int threeLines = subject->height();

    CertificateInfo single = _snapshot.certificates.first();
    single.category = CertificateInfo::Category::Trusted;
    single.subject = QStringLiteral("CN=Single");
    page.setCertificates({ single });
    QCOMPARE(subject->blockCount(), 1);
    QVERIFY(subject->height() < threeLines);
    QCOMPARE(subject->verticalScrollBar()->maximum(), 0);
}

void TestPages::certificateDetailsKeepTheirWidth()
{
    CertificatesPage page;
    page.setCertificates(_snapshot.certificates);
    const auto *scroll = child<QScrollArea>(page, "detailsPanelScroll");
    QVERIFY(scroll->minimumWidth() >= scroll->widget()->minimumSizeHint().width());
    QVERIFY(page.minimumSizeHint().width() >= scroll->minimumWidth());
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
