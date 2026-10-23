// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <cmath>

#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalSpy>
#include <QSpinBox>
#include <QSplitter>
#include <QStandardItemModel>
#include <QStyleOptionViewItem>
#include <QTableView>
#include <QTableWidget>
#include <QTextBlock>
#include <QTextLayout>
#include <QToolButton>
#include <QToolTip>
#include <QTest>
#include <QTreeView>

#include "certificatesmodel.h"
#include "certificatespage.h"
#include "endpointsmodel.h"
#include "endpointspage.h"
#include "formatting.h"
#include "itemroles.h"
#include "logfilterproxymodel.h"
#include "logmodel.h"
#include "logpage.h"
#include "mock/mockdata.h"
#include "namespacesmodel.h"
#include "namespacespage.h"
#include "objectspage.h"
#include "pki/certificategenerator.h"
#include "project/defaultproject.h"
#include "sessionsmodel.h"
#include "sessionspage.h"
#include "statusindicator.h"
#include "statuspage.h"
#include "subscriptionsdialog.h"
#include "subscriptionsmodel.h"
#include "subtypesmodel.h"
#include "typespage.h"
#include "variablesmodel.h"
#include "valuedialog.h"
#include "viewsetup.h"

Q_DECLARE_METATYPE(VariableInfo)
Q_DECLARE_METATYPE(SessionInfo)
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

///
/// \brief Locates a certificate field in the details list.
/// \param page Certificate page.
/// \param name Field label.
/// \return Field row.
///
int certificateFieldRow(const QWidget &page, const char *name)
{
    const auto *view = child<QTableWidget>(page, "detailsView");
    for (int row = 0; row < view->rowCount(); ++row) {
        if (view->item(row, 0)->text() == QLatin1String(name))
            return row;
    }
    qFatal("Missing certificate field %s", name);
    return -1;
}

///
/// \brief Returns a certificate field from the details list.
/// \param page Certificate page.
/// \param name Field label.
/// \return Complete field value.
///
QString certificateField(const QWidget &page, const char *name)
{
    return child<QTableWidget>(page, "detailsView")->item(certificateFieldRow(page, name), 1)
        ->data(Qt::UserRole).toString();
}

///
/// \brief Activates a certificate field and returns its value viewer.
/// \param page Certificate page.
/// \param row Field row.
/// \return Open value dialog.
///
ValueDialog *openCertificateValue(CertificatesPage &page, int row)
{
    auto *view = child<QTableWidget>(page, "detailsView");
    view->activated(view->model()->index(row, 1));
    auto *dialog = page.findChild<ValueDialog *>();
    if (!dialog)
        qFatal("Missing certificate value dialog");
    return dialog;
}

///
/// \brief Closes a viewer and processes its deferred deletion.
/// \param dialog Value viewer.
///
void closeCertificateValue(ValueDialog *dialog)
{
    child<QDialogButtonBox>(*dialog, "buttonBox")->button(QDialogButtonBox::Close)->click();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

using SecurityPair = QPair<QString, QString>;

///
/// \brief Builds an endpoint that only sets its security pair.
/// \param policy Security policy name.
/// \param mode Message security mode name.
/// \return Endpoint with defaults elsewhere.
///
EndpointInfo securedEndpoint(const QString &policy, const QString &mode)
{
    EndpointInfo endpoint;
    endpoint.securityPolicy = policy;
    endpoint.securityMode = mode;
    return endpoint;
}

///
/// \brief Clicks Add on an endpoints page showing the given endpoints.
/// \param endpoints Endpoints configured before the addition.
/// \return Security pair of the added endpoint.
///
SecurityPair addedSecurityPair(const QList<EndpointInfo> &endpoints)
{
    EndpointsPage page;
    page.setEndpoints(endpoints);
    QSignalSpy requested(&page, &EndpointsPage::endpointsChangeRequested);
    child<QPushButton>(page, "addEndpointButton")->click();
    const EndpointInfo added = requested.last().first().value<QList<EndpointInfo>>().last();
    return { added.securityPolicy, added.securityMode };
}

}

class TestPages : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void propertiesActionsStayAtBottom_data();
    void propertiesActionsStayAtBottom();
    void statusPage();
    void objectsPageStartsOnCurrentVariable();
    void objectsPageFollowsVariableSelection();
    void objectsPageShowsObjects();
    void objectsPageClearsForFolders();
    void objectsPageUpdatesValues();
    void objectsPageAppliesEditedProperties();
    void objectsPageRevertsEdits();
    void objectsPageEditsValueList();
    void objectsPageEditsInitialValueAndQuality();
    void objectsPageKeepsSelectionOnReload();
    void objectsPageUpdatesOneVariable();
    void objectsPageUpdatesVariableIcon();
    void objectsPageKeepsUnlistedIcon();
    void objectsPageValueColumnOnlyGrows();
    void objectsPageReportsCurrentNode();
    void typesPage();
    void typesPageShowsAllStandardTypesOnRequest();
    void namespacesPage();
    void endpointsPage();
    void endpointsPageAppliesEdits();
    void endpointsPageAddsAndRemoves();
    void endpointsPageAddsNextSecurityPair();
    void endpointsPageDisablesAddWhenEveryPairIsUsed();
    void endpointsPageRevertsAndKeepsRow();
    void endpointsPageOffersSecurityOfTheBuild();
    void endpointsPageKeepsPolicyAndModePaired();
    void endpointsPageShowsStoredMismatch();
    void endpointsPageFitsEveryEndpoint();
    void endpointsPageRequestsUsers();
    void certificatesPage();
    void certificatesPageRequestsActions();
    void certificatesPageOwnFolderActions();
    void certificatesPageClearsDetailsOfEmptyFolder();
    void certificateDetailsShowEveryLine();
    void certificateDetailsKeepTheirWidth();
    void certificateTechnicalDetails();
    void certificateFieldsHaveSingleLinePreviewsAndIcons();
    void certificateOmitsAbsentExtensions();
    void certificateDetailsUseWholePanel_data();
    void certificateDetailsUseWholePanel();
    void certificateExpandButtonsFollowAvailableSpace();
    void certificateExpandButtonCoversSubpixelOverflow();
    void certificateTruncatedValueTooltip();
    void certificateLongValues_data();
    void certificateLongValues();
    void sessionsPage();
    void sessionsPageRequestsSubscriptions();
    void sessionsPageFiltersByState();
    void logPageFilters();
    void logPageCategoryMenu();
    void logPageSearch();
    void logPageSelectsFirstEntryWhenFilledLater();
    void logPageDetailsAndClear();
    void logPageExportsVisibleEntries();
    void namespacesPageRequestsImportAndExport();
    void namespacesPageRemovesImportedModels();
    void objectsPageShowsImportedVariablesReadOnly();
    void objectsPageLocksModelVariableStructure();
    void objectsPageShowsDataTypeOutsideTheList();

private:
    ServerSnapshot _snapshot;
};

void TestPages::initTestCase()
{
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
    _snapshot = MockData::snapshot();
}

///
/// \brief Covers both property panels with and without vertical overflow.
///
void TestPages::propertiesActionsStayAtBottom_data()
{
    QTest::addColumn<bool>("objects");
    QTest::addColumn<int>("height");
    QTest::newRow("objects-short") << true << 360;
    QTest::newRow("objects-tall") << true << 800;
    QTest::newRow("endpoints-short") << false << 360;
    QTest::newRow("endpoints-tall") << false << 800;
}

///
/// \brief Keeps property actions visible at the panel bottom during scrolling and resizing.
///
void TestPages::propertiesActionsStayAtBottom()
{
    QFETCH(bool, objects);
    QFETCH(int, height);
    ObjectsPage objectsPage;
    EndpointsPage endpointsPage;
    QWidget &page = objects ? static_cast<QWidget &>(objectsPage)
                            : static_cast<QWidget &>(endpointsPage);
    if (objects)
        objectsPage.setAddressSpace(_snapshot.addressSpace, _snapshot.variables);
    else
        endpointsPage.setEndpoints(_snapshot.endpoints);
    page.resize(1200, height);
    page.show();
    QCoreApplication::processEvents();

    auto *panel = child<QWidget>(page, "propertiesPanel");
    auto *scroll = child<QScrollArea>(page, "propertiesPanelScroll");
    auto *apply = child<QPushButton>(page, "applyButton");
    auto *revert = child<QPushButton>(page, "revertButton");
    QVERIFY(apply->isEnabled());
    QVERIFY(revert->isEnabled());
    QVERIFY(!scroll->isAncestorOf(apply));
    QVERIFY(!scroll->isAncestorOf(revert));
    const QPoint position = apply->mapTo(panel, QPoint());
    const int bottomGap = panel->height() - position.y() - apply->height();
    QVERIFY(bottomGap >= 0 && bottomGap <= 12);
    QCOMPARE(revert->mapTo(panel, QPoint()).y(), position.y());

    scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());
    QCoreApplication::processEvents();
    QCOMPARE(apply->mapTo(panel, QPoint()), position);
    page.resize(1200, height + 150);
    QCoreApplication::processEvents();
    QCOMPARE(panel->height() - apply->mapTo(panel, QPoint()).y() - apply->height(), bottomGap);

    child<QTableView>(page, objects ? "variablesView" : "endpointsView")->setCurrentIndex({});
    QVERIFY(!apply->isEnabled());
    QVERIFY(!revert->isEnabled());
}

void TestPages::statusPage()
{
    StatusPage page;
    page.setStatus(_snapshot.status);

    QCOMPARE(child<StatusIndicator>(page, "serverStatusIndicator")->status(), StatusKind::Good);
    QVERIFY(!page.findChild<StatusIndicator *>(QStringLiteral("httpsIndicator")));
    QCOMPARE(child<QLabel>(page, "primaryEndpointValue")->text(), _snapshot.status.primaryEndpoint);
    QCOMPARE(child<QLabel>(page, "uptimeValue")->text(), QStringLiteral("00:27:36"));
    QCOMPARE(child<QLabel>(page, "totalNodesValue")->text(), QString::number(_snapshot.status.totalNodes));
    QCOMPARE(child<QLabel>(page, "clientsValue")->text(), QStringLiteral("1"));
    QCOMPARE(child<QLabel>(page, "securityModesValue")->text(), QStringLiteral("None, Sign, Sign & Encrypt"));
    QVERIFY(!page.findChild<QWidget *>(QStringLiteral("statisticsPanel")));

    const QList<QLabel *> labels = page.findChildren<QLabel *>();
    for (const QLabel *label : labels) {
        if (label->objectName().endsWith(QLatin1String("Value")))
            QVERIFY2(label->textInteractionFlags() & Qt::TextSelectableByMouse,
                     qPrintable(label->objectName()));
    }
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
    QVERIFY(!variable.initialValue.isValid());
    QCOMPARE(variable.configuredQuality, VariableInfo::Quality::Good);
    QCOMPARE(variable.arrayLength, 0);

    auto *arrayLength = child<QSpinBox>(page, "arrayLengthSpin");
    QCOMPARE(arrayLength->text(), QStringLiteral("Scalar"));
    arrayLength->setValue(4);
    child<QPushButton>(page, "applyButton")->click();
    QCOMPARE(requested.last().first().value<VariableInfo>().arrayLength, 4);
}

void TestPages::objectsPageEditsValueList()
{
    ObjectsPage page;
    page.setAddressSpace(_snapshot.addressSpace, _snapshot.variables);
    page.show();
    QSignalSpy requested(&page, &ObjectsPage::variableChangeRequested);
    auto *values = child<QLineEdit>(page, "valuesEdit");
    auto *amplitude = child<QDoubleSpinBox>(page, "amplitudeSpin");
    QVERIFY(!values->isVisible());
    QVERIFY(amplitude->isVisible());

    child<QComboBox>(page, "simulationCombo")->setCurrentText(QStringLiteral("Value list"));
    QVERIFY(values->isVisible());
    QVERIFY(!amplitude->isVisible());
    values->setText(QStringLiteral(" 1; 2 ;;3 "));
    child<QPushButton>(page, "applyButton")->click();

    QCOMPARE(requested.count(), 1);
    const VariableInfo variable = requested.first().first().value<VariableInfo>();
    QCOMPARE(variable.simulation.function, SimulationFunction::List);
    QCOMPARE(variable.simulation.values, QStringList({ QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3") }));
}

void TestPages::objectsPageEditsInitialValueAndQuality()
{
    QHash<QString, QList<VariableInfo>> variables = _snapshot.variables;
    for (QList<VariableInfo> &list : variables) {
        for (VariableInfo &variable : list) {
            variable.initialValue = 1000.0;
            variable.configuredQuality = VariableInfo::Quality::Uncertain;
        }
    }
    ObjectsPage page;
    page.setAddressSpace(_snapshot.addressSpace, variables);
    QSignalSpy requested(&page, &ObjectsPage::variableChangeRequested);

    auto *initialValue = child<QLineEdit>(page, "initialValueEdit");
    auto *quality = child<QComboBox>(page, "qualityCombo");
    QCOMPARE(initialValue->text(), Formatting::value(1000.0));
    QCOMPARE(quality->currentText(), QStringLiteral("Uncertain"));

    initialValue->selectAll();
    QTest::keyClicks(initialValue, QStringLiteral("1500"));
    quality->setCurrentText(QStringLiteral("Bad"));
    child<QPushButton>(page, "applyButton")->click();

    QCOMPARE(requested.count(), 1);
    const VariableInfo variable = requested.first().first().value<VariableInfo>();
    QCOMPARE(variable.initialValue, QVariant(QStringLiteral("1500")));
    QCOMPARE(variable.configuredQuality, VariableInfo::Quality::Bad);

    child<QPushButton>(page, "revertButton")->click();
    QCOMPARE(initialValue->text(), Formatting::value(1000.0));
    QCOMPARE(quality->currentText(), QStringLiteral("Uncertain"));
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
    page.setTypes(_snapshot.types);
    QCOMPARE(text(page, "browseNameEdit"), QStringLiteral("FolderType"));
    QVERIFY(ViewSetup::selectById(child<QTreeView>(page, "typesTree"), QStringLiteral("ns=2;i=1000")));

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

void TestPages::typesPageShowsAllStandardTypesOnRequest()
{
    QHash<QString, TypeInfo> types = _snapshot.types;
    TypeInfo serverStatus;
    serverStatus.browseName = QStringLiteral("ServerStatusType");
    serverStatus.nodeId = QStringLiteral("i=2138");
    serverStatus.baseTypeId = QStringLiteral("i=63");
    serverStatus.category = QStringLiteral("VariableType");
    types.insert(serverStatus.nodeId, serverStatus);
    TypesPage page;
    page.setTypes(types);
    auto *tree = child<QTreeView>(page, "typesTree");
    QVERIFY(!ViewSetup::selectById(tree, serverStatus.nodeId));

    child<QCheckBox>(page, "allTypesCheck")->setChecked(true);
    QVERIFY(ViewSetup::selectById(tree, serverStatus.nodeId));
    QCOMPARE(text(page, "browseNameEdit"), QStringLiteral("ServerStatusType"));

    child<QCheckBox>(page, "allTypesCheck")->setChecked(false);
    QVERIFY(!ViewSetup::selectById(tree, serverStatus.nodeId));
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
    QCOMPARE(text(page, "hostnameEdit"), QStringLiteral("localhost"));
    QCOMPARE(child<QComboBox>(page, "securityModeCombo")->currentText(), QStringLiteral("Sign & Encrypt"));
    QVERIFY(!page.findChild<QLineEdit *>(QStringLiteral("usernameEdit")));

    ViewSetup::selectRow(child<QTableView>(page, "endpointsView"), 3);
    QCOMPARE(text(page, "hostnameEdit"), QStringLiteral("localhost"));
    QCOMPARE(child<QComboBox>(page, "securityPolicyCombo")->currentText(), QStringLiteral("None"));

    child<QLineEdit>(page, "searchEdit")->setText(QStringLiteral("192.168"));
    ViewSetup::selectRow(child<QTableView>(page, "endpointsView"), 0);
    QCOMPARE(text(page, "hostnameEdit"), QStringLiteral("192.168.1.100"));
}

void TestPages::endpointsPageAppliesEdits()
{
    const QList<EndpointInfo> endpoints = _snapshot.endpoints;
    EndpointsPage page;
    page.setEndpoints(endpoints);
    QSignalSpy requested(&page, &EndpointsPage::endpointsChangeRequested);

    child<QLineEdit>(page, "hostnameEdit")->setText(QStringLiteral("edited-host"));
    child<QSpinBox>(page, "portSpin")->setValue(4900);
    child<QCheckBox>(page, "usernamePasswordCheck")->setChecked(true);
    child<QPushButton>(page, "applyButton")->click();

    QCOMPARE(requested.count(), 1);
    using Endpoints = QList<EndpointInfo>;
    const Endpoints applied = requested.first().first().value<Endpoints>();
    QCOMPARE(applied.size(), endpoints.size());
    QCOMPARE(applied[0].hostname, QStringLiteral("edited-host"));
    QCOMPARE(applied[0].port, 4900);
    QVERIFY(applied[0].usernamePassword);
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
    QCOMPARE(added.last().hostname, _snapshot.endpoints.first().hostname);
    QCOMPARE(added.last().port, _snapshot.endpoints.first().port);

    ViewSetup::selectRow(child<QTableView>(page, "endpointsView"), 1);
    child<QPushButton>(page, "removeEndpointButton")->click();
    const Endpoints removed = requested.last().first().value<Endpoints>();
    QCOMPARE(removed.size(), _snapshot.endpoints.size() - 1);
    QCOMPARE(removed[1].hostname, _snapshot.endpoints[2].hostname);

    page.setEndpoints({});
    QVERIFY(!child<QPushButton>(page, "removeEndpointButton")->isEnabled());
}

void TestPages::endpointsPageAddsNextSecurityPair()
{
    const QString none = QStringLiteral("None");
    const QString sign = QStringLiteral("Sign");
    const QString encrypt = QStringLiteral("Sign & Encrypt");
    const QString basic128 = QStringLiteral("Basic128Rsa15");
    const QString basic256 = QStringLiteral("Basic256");

    QCOMPARE(addedSecurityPair({}), SecurityPair(none, none));
#ifdef OUASIM_HAVE_ENCRYPTION
    QCOMPARE(addedSecurityPair({ securedEndpoint(none, none) }), SecurityPair(basic128, sign));
    QCOMPARE(addedSecurityPair({ securedEndpoint(basic256, sign), securedEndpoint(basic128, encrypt) }),
             SecurityPair(basic256, encrypt));
    QCOMPARE(addedSecurityPair({ securedEndpoint(basic128, sign),
                                 securedEndpoint(QStringLiteral("Aes256_Sha256_RsaPss"), encrypt) }),
             SecurityPair(none, none));
#endif
}

void TestPages::endpointsPageDisablesAddWhenEveryPairIsUsed()
{
    EndpointsPage page;
    auto *add = child<QPushButton>(page, "addEndpointButton");
    page.setEndpoints({});
    QVERIFY(add->isEnabled());

#ifdef OUASIM_HAVE_ENCRYPTION
    QList<EndpointInfo> endpoints = DefaultProject::create().endpoints;
#else
    QList<EndpointInfo> endpoints = { securedEndpoint(QStringLiteral("None"), QStringLiteral("None")) };
#endif
    page.setEndpoints(endpoints);
    QVERIFY(!add->isEnabled());

    endpoints.last().securityMode = endpoints.last().securityPolicy == QLatin1String("None")
                                        ? QStringLiteral("Sign")
                                        : QStringLiteral("None");
    page.setEndpoints(endpoints);
    QVERIFY(add->isEnabled());
}

void TestPages::endpointsPageRevertsAndKeepsRow()
{
    EndpointsPage page;
    page.setEndpoints(_snapshot.endpoints);
    ViewSetup::selectRow(child<QTableView>(page, "endpointsView"), 2);
    child<QLineEdit>(page, "hostnameEdit")->setText(QStringLiteral("changed-host"));
    child<QPushButton>(page, "revertButton")->click();
    QCOMPARE(text(page, "hostnameEdit"), _snapshot.endpoints[2].hostname);

    page.setEndpoints(_snapshot.endpoints);
    QCOMPARE(text(page, "hostnameEdit"), _snapshot.endpoints[2].hostname);
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

void TestPages::endpointsPageFitsEveryEndpoint()
{
    const QList<EndpointInfo> endpoints = DefaultProject::create().endpoints;
    EndpointsPage page;
    page.setEndpoints({ endpoints.first() });
    page.resize(1200, 700);
    page.show();
    QVERIFY(QTest::qWaitForWindowExposed(&page));
    page.setEndpoints(endpoints);

    auto *table = child<QTableView>(page, "endpointsView");
    const QAbstractItemView *view = table;
    for (int column = 0; column < EndpointsModel::ColEnabled; ++column)
        QVERIFY2(table->horizontalHeader()->sectionSize(column) >= view->sizeHintForColumn(column),
                 qPrintable(QStringLiteral("column %1").arg(column)));
}

void TestPages::endpointsPageRequestsUsers()
{
    EndpointsPage page;
    page.setEndpoints(_snapshot.endpoints);
    QSignalSpy requested(&page, &EndpointsPage::usersEditRequested);
    child<QPushButton>(page, "usersButton")->click();
    QCOMPARE(requested.count(), 1);
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
    QVERIFY(certificateField(page, "Subject").startsWith(QStringLiteral("CN=OPC UA Client A")));
    QCOMPARE(certificateField(page, "Valid from"), QStringLiteral("2024-04-10 10:00:00"));
    QVERIFY(!child<QPushButton>(page, "trustButton")->isEnabled());
    QVERIFY(child<QPushButton>(page, "rejectButton")->isEnabled());

    const QString rejected = QString::number(static_cast<int>(CertificateInfo::Category::Rejected));
    QVERIFY(ViewSetup::selectById(child<QTreeView>(page, "categoriesList"), rejected));
    QCOMPARE(page.certificatesModel()->rowCount(), 1);
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

void TestPages::certificatesPageClearsDetailsOfEmptyFolder()
{
    QList<CertificateInfo> ownOnly;
    for (const CertificateInfo &certificate : std::as_const(_snapshot.certificates)) {
        if (certificate.category == CertificateInfo::Category::Application)
            ownOnly.append(certificate);
    }
    QVERIFY(!ownOnly.isEmpty());
    CertificatesPage page;
    page.setCertificates(ownOnly);
    auto *categories = child<QTreeView>(page, "categoriesList");

    QVERIFY(ViewSetup::selectById(categories, QString::number(int(CertificateInfo::Category::Application))));
    QVERIFY(child<QTableWidget>(page, "detailsView")->rowCount() > 0);
    QVERIFY(ViewSetup::selectById(categories, QString::number(int(CertificateInfo::Category::Trusted))));
    QCOMPARE(child<QTableWidget>(page, "detailsView")->rowCount(), 0);
}

void TestPages::certificatesPageOwnFolderActions()
{
    CertificatesPage page;
    page.setCertificates(_snapshot.certificates);
    page.show();
    auto *generate = child<QPushButton>(page, "generateButton");
    auto *categories = child<QTreeView>(page, "categoriesList");
    QSignalSpy requested(&page, &CertificatesPage::generateRequested);

    QVERIFY(ViewSetup::selectById(categories, QString::number(int(CertificateInfo::Category::Trusted))));
    QVERIFY(!generate->isVisible());
    QVERIFY(child<QPushButton>(page, "trustButton")->isVisible());
    QVERIFY(child<QPushButton>(page, "rejectButton")->isVisible());
    QVERIFY(ViewSetup::selectById(categories, QString::number(int(CertificateInfo::Category::Application))));
    QVERIFY(generate->isVisible());
    QVERIFY(!child<QPushButton>(page, "trustButton")->isVisible());
    QVERIFY(!child<QPushButton>(page, "rejectButton")->isVisible());
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

    auto *view = child<QTableWidget>(page, "detailsView");
    for (const char *field : { "Subject", "Issuer" }) {
        const int row = certificateFieldRow(page, field);
        auto *dialog = openCertificateValue(page, row);
        auto *edit = child<QPlainTextEdit>(*dialog, "valueEdit");
        QCOMPARE(edit->blockCount(), 3);
        QCOMPARE(edit->toPlainText(), view->item(row, 1)->data(Qt::UserRole).toString());
        closeCertificateValue(dialog);
    }

    CertificateInfo single = _snapshot.certificates.first();
    single.category = CertificateInfo::Category::Trusted;
    single.subject = QStringLiteral("CN=Single");
    view->selectRow(certificateFieldRow(page, "Subject"));
    page.setCertificates({ single });
    auto *dialog = openCertificateValue(page, certificateFieldRow(page, "Subject"));
    auto *edit = child<QPlainTextEdit>(*dialog, "valueEdit");
    QCOMPARE(edit->blockCount(), 1);
    QCOMPARE(edit->toPlainText(), single.subject);
}

void TestPages::certificateDetailsKeepTheirWidth()
{
    CertificatesPage page;
    page.setCertificates(_snapshot.certificates);
    const auto *scroll = child<QScrollArea>(page, "detailsPanelScroll");
    QVERIFY(scroll->minimumWidth() >= scroll->widget()->minimumSizeHint().width());
    QVERIFY(page.minimumSizeHint().width() >= scroll->minimumWidth());
}

///
/// \brief Shows technical certificate fields and clears them when selection becomes empty.
///
void TestPages::certificateTechnicalDetails()
{
    CertificateInfo certificate = _snapshot.certificates.first();
    certificate.category = CertificateInfo::Category::Trusted;
    certificate.serialNumber = QStringLiteral("01");
    certificate.sha256Fingerprint = QStringLiteral("AA:BB:CC");
    certificate.details = QStringLiteral("Version: 3\nSignature Algorithm: sha256WithRSAEncryption\nURI:urn:test");
    CertificatesPage page;
    page.setCertificates({ certificate });
    QCOMPARE(certificateField(page, "Serial Number"), QStringLiteral("01"));
    QCOMPARE(certificateField(page, "Version"), certificate.version);
    QCOMPARE(certificateField(page, "Signature Algorithm"), certificate.signatureAlgorithm);
    QCOMPARE(certificateField(page, "Signature Hash Algorithm"), certificate.signatureHashAlgorithm);
    QCOMPARE(certificateField(page, "Public Key"), certificate.publicKey);
    QCOMPARE(certificateField(page, "Public Key Parameters"), certificate.publicKeyParameters);
    QCOMPARE(certificateField(page, "Basic Constraints"), certificate.basicConstraints);
    QCOMPARE(certificateField(page, "Extended Key Usage"), certificate.extendedKeyUsage);
    QCOMPARE(certificateField(page, "Subject Key Identifier"), certificate.subjectKeyIdentifier);
    QCOMPARE(certificateField(page, "Authority Key Identifier"), certificate.authorityKeyIdentifier);
    QCOMPARE(certificateField(page, "Subject Alternative Names"), certificate.subjectAlternativeNames);
    QCOMPARE(certificateField(page, "SHA-1 Thumbprint"), certificate.fingerprint);
    QCOMPARE(certificateField(page, "SHA-256 Fingerprint"), certificate.sha256Fingerprint);
    QCOMPARE(certificateField(page, "X.509 Details"), certificate.details);
    auto *view = child<QTableWidget>(page, "detailsView");
    auto *dialog = openCertificateValue(page, view->rowCount() - 1);
    auto *edit = child<QPlainTextEdit>(*dialog, "valueEdit");
    QCOMPARE(edit->toPlainText(), certificate.details);
    QVERIFY(edit->isReadOnly());
    QCOMPARE(view->editTriggers(), QAbstractItemView::NoEditTriggers);
    page.setCertificates({});
    QCOMPARE(view->rowCount(), 0);
    QCOMPARE(edit->toPlainText(), certificate.details);
}

///
/// \brief Keeps field previews on one line while preserving multiline values and field decorations.
///
void TestPages::certificateFieldsHaveSingleLinePreviewsAndIcons()
{
    CertificateInfo certificate = _snapshot.certificates.first();
    certificate.category = CertificateInfo::Category::Trusted;
    certificate.subject = QStringLiteral("CN=Server,\nO=Example\tCompany,\nC=US");
    certificate.details = QStringLiteral("Certificate:\n    Data:\r\n        Version: 3\n") + QString(300, QLatin1Char('A'));
    CertificatesPage page;
    page.setCertificates({ certificate });
    page.resize(1100, 600);
    page.show();
    QVERIFY(QTest::qWaitForWindowExposed(&page));
    child<QSplitter>(page, "splitter")->setSizes({ 760, 340 });
    auto *view = child<QTableWidget>(page, "detailsView");
    QCOMPARE(view->rowCount(), 20);
    QVERIFY(!view->wordWrap());
    QCOMPARE(view->textElideMode(), Qt::ElideRight);
    for (int row = 0; row < view->rowCount(); ++row) {
        const QTableWidgetItem *field = view->item(row, 0);
        const QTableWidgetItem *preview = view->item(row, 1);
        QVERIFY(field->text() != QStringLiteral("Status"));
        QVERIFY(!field->icon().isNull());
        QVERIFY(!field->icon().pixmap(16, 16).isNull());
        QCOMPARE(preview->text(), preview->data(Qt::UserRole).toString().simplified());
        QVERIFY(!preview->data(ItemRoles::Status).isValid());
        QCOMPARE(view->rowHeight(row), view->rowHeight(0));
    }
    const int subjectRow = certificateFieldRow(page, "Subject");
    QCOMPARE(view->item(subjectRow, 1)->text(), QStringLiteral("CN=Server, O=Example Company, C=US"));
    auto *dialog = openCertificateValue(page, subjectRow);
    auto *value = child<QPlainTextEdit>(*dialog, "valueEdit");
    QCOMPARE(value->toPlainText(), certificate.subject);
    value->selectAll();
    value->copy();
    QCOMPARE(QApplication::clipboard()->text(), certificate.subject);
    const QString preview = view->item(view->rowCount() - 1, 1)->text();
    const int width = view->columnWidth(1) - 10;
    QVERIFY(view->fontMetrics().horizontalAdvance(preview) > width);
    QVERIFY(view->fontMetrics().elidedText(preview, view->textElideMode(), width).endsWith(QChar(0x2026)));
}

///
/// \brief Leaves absent optional certificate fields out of the list.
///
void TestPages::certificateOmitsAbsentExtensions()
{
    CertificateInfo certificate;
    certificate.category = CertificateInfo::Category::Trusted;
    CertificatesPage page;
    page.setCertificates({ certificate });
    auto *view = child<QTableWidget>(page, "detailsView");
    QCOMPARE(view->rowCount(), 13);
    for (const char *field : { "Public Key Parameters", "Basic Constraints", "Key Usage", "Extended Key Usage",
                              "Subject Key Identifier", "Authority Key Identifier", "Subject Alternative Names", "Status" }) {
        QVERIFY(view->findItems(QLatin1String(field), Qt::MatchExactly).isEmpty());
    }
}

///
/// \brief Covers compact and tall certificate panels.
///
void TestPages::certificateDetailsUseWholePanel_data()
{
    QTest::addColumn<int>("height");
    QTest::newRow("compact") << 470;
    QTest::newRow("tall") << 800;
}

///
/// \brief Gives the field list all available space without a lower value pane.
///
void TestPages::certificateDetailsUseWholePanel()
{
    QFETCH(int, height);
    CertificatesPage page;
    page.setCertificates(_snapshot.certificates);
    page.resize(1100, height);
    page.show();
    QVERIFY(QTest::qWaitForWindowExposed(&page));
    QVERIFY(!page.findChild<QPlainTextEdit *>(QStringLiteral("selectedValueEdit")));
    QVERIFY(!page.findChild<QSplitter *>(QStringLiteral("detailsSplitter")));
    auto *view = child<QTableWidget>(page, "detailsView");
    QCOMPARE(view->height(), child<QWidget>(page, "detailsHost")->height());
    const int initialHeight = view->height();
    page.resize(1100, height + 150);
    QCoreApplication::processEvents();
    QVERIFY(view->height() > initialHeight);
}

///
/// \brief Offers expansion only while a single-line value does not fit.
///
void TestPages::certificateExpandButtonsFollowAvailableSpace()
{
    CertificateInfo certificate = _snapshot.certificates.first();
    certificate.category = CertificateInfo::Category::Trusted;
    certificate.serialNumber = QStringLiteral("01 23 45 67 89 AB CD EF");
    CertificatesPage page;
    page.setCertificates({ certificate });
    page.resize(1300, 600);
    page.show();
    QVERIFY(QTest::qWaitForWindowExposed(&page));
    auto *view = child<QTableWidget>(page, "detailsView");
    view->horizontalHeader()->setStretchLastSection(false);
    view->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    const QModelIndex index = view->model()->index(certificateFieldRow(page, "Serial Number"), 1);
    view->setColumnWidth(1, 300);
    view->scrollTo(index);
    QCoreApplication::processEvents();
    QRect rect = view->visualRect(index);
    QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(rect.right() - 12, rect.center().y()));
    QVERIFY(!page.findChild<ValueDialog *>());
    view->setColumnWidth(1, 80);
    view->scrollTo(index);
    QCoreApplication::processEvents();
    rect = view->visualRect(index);
    QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(rect.right() - 12, rect.center().y()));
    auto *dialog = page.findChild<ValueDialog *>();
    QVERIFY(dialog);
    QCOMPARE(child<QPlainTextEdit>(*dialog, "valueEdit")->toPlainText(), certificate.serialNumber);
    closeCertificateValue(dialog);
    view->setColumnWidth(1, 300);
    view->scrollTo(index);
    QCoreApplication::processEvents();
    rect = view->visualRect(index);
    QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(rect.right() - 12, rect.center().y()));
    QVERIFY(!page.findChild<ValueDialog *>());
    for (const auto key : { Qt::Key_Return, Qt::Key_Enter }) {
        page.activateWindow();
        view->setCurrentIndex(index);
        view->setFocus();
        QTRY_VERIFY(view->hasFocus());
        QTest::keyClick(view, key);
        auto *keyboardDialog = page.findChild<ValueDialog *>();
        QVERIFY(keyboardDialog);
        QCOMPARE(page.findChildren<ValueDialog *>().size(), 1);
        QCOMPARE(child<QPlainTextEdit>(*keyboardDialog, "valueEdit")->toPlainText(), certificate.serialNumber);
        closeCertificateValue(keyboardDialog);
    }
}

///
/// \brief Offers expansion when the style elides a value that overflows by less than half a pixel.
///
void TestPages::certificateExpandButtonCoversSubpixelOverflow()
{
    CertificatesPage page;
    page.resize(1300, 600);
    page.show();
    QVERIFY(QTest::qWaitForWindowExposed(&page));
    auto *view = child<QTableWidget>(page, "detailsView");
    QFont font = view->font();
    font.setPointSizeF(9.37);
    font.setHintingPreference(QFont::PreferNoHinting);
    view->setFont(font);
    QString subject;
    qreal width = 0;
    for (int length = 1; length < 64 && subject.isEmpty(); ++length) {
        const QString candidate = QStringLiteral("CN=OpenUaSim@") + QString(length, QLatin1Char('A' + length % 26));
        width = QFontMetricsF(view->font()).horizontalAdvance(candidate);
        const qreal fraction = width - std::floor(width);
        if (fraction > 0.05 && fraction < 0.45)
            subject = candidate;
    }
    if (subject.isEmpty())
        QSKIP("The font has no fractional advances to overflow by a subpixel");

    CertificateInfo certificate = _snapshot.certificates.first();
    certificate.category = CertificateInfo::Category::Trusted;
    certificate.subject = subject;
    page.setCertificates({ certificate });
    view->horizontalHeader()->setStretchLastSection(false);
    view->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    view->setColumnWidth(1, 600);
    const QModelIndex index = view->model()->index(certificateFieldRow(page, "Subject"), 1);
    view->scrollTo(index);
    QCoreApplication::processEvents();

    QStyleOptionViewItem option;
    option.initFrom(view);
    option.rect = view->visualRect(index);
    option.text = subject;
    option.features = QStyleOptionViewItem::HasDisplay;
    option.widget = view;
    const QRect textRect = view->style()->subElementRect(QStyle::SE_ItemViewItemText, &option, view);
    const int margin = view->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, &option, view) + 1;
    const int available = textRect.width() - 2 * margin;
    view->setColumnWidth(1, view->columnWidth(1) - available + int(std::floor(width)));
    view->scrollTo(index);
    QCoreApplication::processEvents();

    const QRect rect = view->visualRect(index);
    QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(rect.right() - 12, rect.center().y()));
    auto *dialog = page.findChild<ValueDialog *>();
    QVERIFY(dialog);
    QCOMPARE(child<QPlainTextEdit>(*dialog, "valueEdit")->toPlainText(), subject);
    closeCertificateValue(dialog);
}

///
/// \brief Shows the complete value as a tooltip only while its preview is truncated.
///
void TestPages::certificateTruncatedValueTooltip()
{
    CertificateInfo certificate = _snapshot.certificates.first();
    certificate.category = CertificateInfo::Category::Trusted;
    certificate.subject = QStringLiteral("CN=<Server> & Co\nO=Example");
    CertificatesPage page;
    page.setCertificates({ certificate });
    page.resize(1300, 600);
    page.show();
    QVERIFY(QTest::qWaitForWindowExposed(&page));
    auto *view = child<QTableWidget>(page, "detailsView");
    view->horizontalHeader()->setStretchLastSection(false);
    view->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    const QModelIndex index = view->model()->index(certificateFieldRow(page, "Subject"), 1);
    const auto hover = [&] {
        view->scrollTo(index);
        QCoreApplication::processEvents();
        const QPoint pos = view->visualRect(index).center();
        QHelpEvent event(QEvent::ToolTip, pos, view->viewport()->mapToGlobal(pos));
        QCoreApplication::sendEvent(view->viewport(), &event);
    };

    view->setColumnWidth(1, 60);
    hover();
    QTRY_VERIFY(QToolTip::isVisible());
    QVERIFY(QToolTip::text().contains(certificate.subject.toHtmlEscaped()));

    certificate.subject = QStringLiteral("CN=Server");
    page.setCertificates({ certificate });
    view->setColumnWidth(1, 600);
    hover();
    QTRY_VERIFY(!QToolTip::isVisible());
}

///
/// \brief Covers long values with and without natural wrapping boundaries.
///
void TestPages::certificateLongValues_data()
{
    QTest::addColumn<QString>("field");
    QTest::addColumn<QString>("value");
    QTest::newRow("subject") << QStringLiteral("Subject") << QStringLiteral("CN=") + QString(300, QLatin1Char('A'));
    QTest::newRow("serial") << QStringLiteral("Serial Number") << QStringLiteral("A1 B2 ").repeated(30).trimmed();
    QTest::newRow("usage") << QStringLiteral("Key Usage") << QStringLiteral("Digital Signature, Key Encipherment, ").repeated(10);
    QTest::newRow("path") << QStringLiteral("File Path") << QStringLiteral("C:/") + QString(300, QLatin1Char('a')) + QStringLiteral("/server.der");
    QTest::newRow("sha256") << QStringLiteral("SHA-256 Fingerprint") << QStringLiteral("A1:").repeated(31) + QStringLiteral("A1");
    QTest::newRow("sha1") << QStringLiteral("SHA-1 Thumbprint") << QStringLiteral("B2:").repeated(19) + QStringLiteral("B2");
    QTest::newRow("multiline") << QStringLiteral("Subject") << QStringLiteral("CN=Server\nO=Example");
    QTest::newRow("whitespace") << QStringLiteral("Subject") << QStringLiteral(" CN=Server\t  O=Example ");
}

///
/// \brief Keeps long values readable and copyable when the details panel is narrow.
///
void TestPages::certificateLongValues()
{
    QFETCH(QString, field);
    QFETCH(QString, value);
    CertificateInfo certificate = _snapshot.certificates.first();
    certificate.category = CertificateInfo::Category::Trusted;
    certificate.subject = value;
    certificate.serialNumber = value;
    certificate.keyUsage = value;
    certificate.filePath = value;
    certificate.sha256Fingerprint = value;
    certificate.fingerprint = value;
    CertificatesPage page;
    page.resize(1100, 600);
    page.setCertificates({ certificate });
    page.show();
    QVERIFY(QTest::qWaitForWindowExposed(&page));
    child<QSplitter>(page, "splitter")->setSizes({ 760, 340 });
    auto *view = child<QTableWidget>(page, "detailsView");
    view->horizontalHeader()->setStretchLastSection(false);
    view->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    view->setColumnWidth(1, 140);
    const int row = certificateFieldRow(page, qPrintable(field));
    const QModelIndex index = view->model()->index(row, 1);
    view->scrollTo(index);
    QCoreApplication::processEvents();
    const QRect rect = view->visualRect(index);
    QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(rect.right() - 12, rect.center().y()));
    auto *dialog = page.findChild<ValueDialog *>();
    QVERIFY(dialog);
    auto *edit = child<QPlainTextEdit>(*dialog, "valueEdit");
    QCOMPARE(edit->toPlainText(), value);
    QCOMPARE(dialog->windowTitle(), field);
    QVERIFY(edit->isReadOnly());
    QCOMPARE(edit->horizontalScrollBar()->maximum(), 0);
    child<QPushButton>(*dialog, "copyButton")->click();
    QCOMPARE(QApplication::clipboard()->text(), value);
    closeCertificateValue(dialog);
    QVERIFY(!page.findChild<ValueDialog *>());
    certificate.subject = QStringLiteral("CN=Replacement");
    page.setCertificates({ certificate });
    QCOMPARE(view->currentRow(), row);
    dialog = openCertificateValue(page, row);
    QCOMPARE(child<QPlainTextEdit>(*dialog, "valueEdit")->toPlainText(), view->item(row, 1)->data(Qt::UserRole).toString());
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

void TestPages::sessionsPageRequestsSubscriptions()
{
    QList<SessionInfo> sessions = _snapshot.sessions;
    SubscriptionInfo subscription;
    subscription.id = 11;
    sessions.first().subscriptionList = { subscription };
    SessionsPage page;
    page.setSessions(sessions);
    auto *button = child<QToolButton>(page, "showSubscriptionsButton");
    QSignalSpy requested(&page, &SessionsPage::subscriptionsRequested);

    QVERIFY(button->isEnabled());
    button->click();
    QCOMPARE(requested.count(), 1);
    const SessionInfo session = requested.first().first().value<SessionInfo>();
    QCOMPARE(session.sessionId, sessions.first().sessionId);

    SubscriptionsDialog dialog(session);
    QCOMPARE(dialog.subscriptionsModel()->rowCount(), 1);
    QVERIFY(child<QLabel>(dialog, "sessionLabel")->text().contains(session.sessionId));

    ViewSetup::selectRow(child<QTableView>(page, "sessionsView"), 1);
    QVERIFY(!button->isEnabled());
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
    child<QAction>(page, "securityAction")->setChecked(false);
    QCOMPARE(page.filterModel()->rowCount(), 1);

    child<QCheckBox>(page, "infoCheck")->setChecked(true);
    child<QAction>(page, "securityAction")->setChecked(true);
    QCOMPARE(page.filterModel()->rowCount(), total);

    child<QAction>(page, "serverAction")->setChecked(false);
    QCOMPARE(page.filterModel()->rowCount(), total - 3);
    child<QAction>(page, "serverAction")->setChecked(true);
    QCOMPARE(page.filterModel()->rowCount(), total);
}

void TestPages::logPageCategoryMenu()
{
    LogPage page;
    const QMenu *menu = child<QPushButton>(page, "categoryButton")->menu();
    QVERIFY(menu);
    QCOMPARE(menu->actions(), QList<QAction *>({ child<QAction>(page, "serverAction"),
                                                 child<QAction>(page, "securityAction"),
                                                 child<QAction>(page, "sessionsAction"),
                                                 child<QAction>(page, "dataChangesAction") }));
    for (const QAction *action : menu->actions())
        QVERIFY(action->isCheckable() && action->isChecked());
}

void TestPages::logPageSearch()
{
    LogModel model;
    model.setItems(_snapshot.log);
    LogPage page;
    page.setLogModel(&model);
    auto *search = child<QLineEdit>(page, "searchEdit");

    search->setText(QStringLiteral("MOTOR_01"));
    QCOMPARE(page.filterModel()->rowCount(), 3);
    QCOMPARE(text(page, "timeEdit"), QStringLiteral("2024-05-23 14:31:27.104"));

    child<QAction>(page, "dataChangesAction")->setChecked(false);
    QCOMPARE(page.filterModel()->rowCount(), 0);
    QVERIFY(!child<QWidget>(page, "detailsHost")->isEnabled());

    child<QAction>(page, "dataChangesAction")->setChecked(true);
    search->clear();
    QCOMPARE(page.filterModel()->rowCount(), model.rowCount());
}

void TestPages::logPageSelectsFirstEntryWhenFilledLater()
{
    LogModel model;
    LogPage page;
    page.setLogModel(&model);
    QVERIFY(text(page, "timeEdit").isEmpty());

    model.setItems(_snapshot.log);
    QCOMPARE(child<QTableView>(page, "logView")->currentIndex().row(), 0);
    QCOMPARE(text(page, "timeEdit"), QStringLiteral("2024-05-23 14:31:20.004"));
}

void TestPages::logPageDetailsAndClear()
{
    LogModel model;
    model.setItems(_snapshot.log);
    LogPage page;
    page.setLogModel(&model);

    QCOMPARE(text(page, "timeEdit"), QStringLiteral("2024-05-23 14:31:20.004"));
    QCOMPARE(text(page, "sourceEdit"), QStringLiteral("OpenUaSim"));
    QCOMPARE(child<StatusIndicator>(page, "levelIndicator")->text(), QStringLiteral("INFO"));

    child<QToolButton>(page, "clearLogButton")->click();
    QCOMPARE(model.rowCount(), 0);
    QVERIFY(text(page, "timeEdit").isEmpty());
    QVERIFY(!child<QWidget>(page, "detailsHost")->isEnabled());
}

void TestPages::logPageExportsVisibleEntries()
{
    LogModel model;
    model.setItems(_snapshot.log);
    LogPage page;
    page.setLogModel(&model);
    child<QCheckBox>(page, "infoCheck")->setChecked(false);
    QSignalSpy requested(&page, &LogPage::exportRequested);

    child<QToolButton>(page, "exportLogButton")->click();
    QCOMPARE(requested.count(), 1);
    const auto entries = requested.at(0).at(0).value<QList<LogEntry>>();
    QCOMPARE(entries.size(), page.filterModel()->rowCount());
    for (const LogEntry &entry : entries)
        QVERIFY(entry.level != LogEntry::Level::Info);
}

void TestPages::namespacesPageRequestsImportAndExport()
{
    NamespacesPage page;
    page.setNamespaces(_snapshot.namespaces);
    auto *exportButton = child<QPushButton>(page, "exportButton");
    QSignalSpy imports(&page, &NamespacesPage::importRequested);
    QSignalSpy exports(&page, &NamespacesPage::exportRequested);

    child<QPushButton>(page, "importButton")->click();
    QCOMPARE(imports.count(), 1);

    QVERIFY(!exportButton->isEnabled());
    exportButton->click();
    QCOMPARE(exports.count(), 0);

    ViewSetup::selectRow(child<QTableView>(page, "namespacesView"), 1);
    QVERIFY(exportButton->isEnabled());
    exportButton->click();
    QCOMPARE(exports.count(), 1);
    QCOMPARE(exports.at(0).at(0).toString(), _snapshot.namespaces.at(1).uri);
}

void TestPages::namespacesPageRemovesImportedModels()
{
    QList<NamespaceInfo> namespaces = _snapshot.namespaces;
    namespaces.last().modelFile = QStringLiteral("/models/robot.xml");
    NamespacesPage page;
    page.setNamespaces(namespaces);
    auto *remove = child<QPushButton>(page, "removeModelButton");
    auto *view = child<QTableView>(page, "namespacesView");
    QSignalSpy requested(&page, &NamespacesPage::modelRemovalRequested);

    QVERIFY(!remove->isEnabled());
    ViewSetup::selectRow(view, 1);
    QVERIFY(!remove->isEnabled());
    ViewSetup::selectRow(view, int(namespaces.size()) - 1);
    QVERIFY(remove->isEnabled());
    remove->click();
    QCOMPARE(requested.count(), 1);
    QCOMPARE(requested.first().first().toString(), QStringLiteral("/models/robot.xml"));
}

void TestPages::objectsPageShowsImportedVariablesReadOnly()
{
    QHash<QString, QList<VariableInfo>> variables = _snapshot.variables;
    for (QList<VariableInfo> &list : variables) {
        for (VariableInfo &variable : list)
            variable.readOnly = true;
    }
    ObjectsPage page;
    page.setAddressSpace(_snapshot.addressSpace, variables);
    QVERIFY(!child<QWidget>(page, "propertiesHost")->isEnabled());
    QCOMPARE(child<QLineEdit>(page, "displayNameEdit")->text(), QStringLiteral("Speed"));
    QVERIFY(!child<QPushButton>(page, "applyButton")->isEnabled());
    QVERIFY(!child<QPushButton>(page, "revertButton")->isEnabled());
}

void TestPages::objectsPageLocksModelVariableStructure()
{
    QHash<QString, QList<VariableInfo>> variables = _snapshot.variables;
    for (QList<VariableInfo> &list : variables) {
        for (VariableInfo &variable : list)
            variable.fromModel = true;
    }
    ObjectsPage page;
    page.setAddressSpace(_snapshot.addressSpace, variables);
    QVERIFY(child<QWidget>(page, "propertiesHost")->isEnabled());
    QVERIFY(child<QPushButton>(page, "applyButton")->isEnabled());
    QVERIFY(!child<QLineEdit>(page, "displayNameEdit")->isEnabled());
    QVERIFY(!child<QComboBox>(page, "dataTypeCombo")->isEnabled());
    QVERIFY(!child<QSpinBox>(page, "arrayLengthSpin")->isEnabled());
    QVERIFY(child<QComboBox>(page, "simulationCombo")->isEnabled());
    QVERIFY(child<QLineEdit>(page, "initialValueEdit")->isEnabled());

    page.setAddressSpace(_snapshot.addressSpace, _snapshot.variables);
    QVERIFY(child<QLineEdit>(page, "displayNameEdit")->isEnabled());
    QVERIFY(child<QComboBox>(page, "dataTypeCombo")->isEnabled());
}

void TestPages::objectsPageShowsDataTypeOutsideTheList()
{
    ObjectsPage page;
    page.setAddressSpace(_snapshot.addressSpace, _snapshot.variables);
    auto *dataType = child<QComboBox>(page, "dataTypeCombo");
    QCOMPARE(dataType->currentText(), QStringLiteral("Double"));

    QHash<QString, QList<VariableInfo>> variables = _snapshot.variables;
    for (QList<VariableInfo> &list : variables) {
        for (VariableInfo &variable : list) {
            variable.dataType = QStringLiteral("Int32");
            variable.dataTypeName = QStringLiteral("TankMode");
        }
    }
    page.setAddressSpace(_snapshot.addressSpace, variables);
    QCOMPARE(dataType->currentIndex(), -1);
    QCOMPARE(dataType->placeholderText(), QStringLiteral("TankMode"));

    page.setAddressSpace(_snapshot.addressSpace, _snapshot.variables);
    QCOMPARE(dataType->currentText(), QStringLiteral("Double"));
    QVERIFY(dataType->placeholderText().isEmpty());
}

QTEST_MAIN(TestPages)
#include "test_pages.moc"
