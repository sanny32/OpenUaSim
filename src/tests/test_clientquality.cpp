// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>
#include <QUaServer>

#include "mainwindow.h"
#include "objectspage.h"
#include "project/projectnodes.h"
#include "server/servercontroller.h"
#include "statusindicator.h"
#include "testport.h"
#include "testproject.h"
#include "uatestclient.h"
#include "variablesmodel.h"

namespace {

const QString kValueId = QStringLiteral("ns=1;s=Test.Value");
const QString kOtherId = QStringLiteral("ns=1;s=Test.Other");

///
/// \brief Creates a writable variable whose initial quality awaits a client value.
///
ProjectConfig project()
{
    auto config = TestProject::create();
    config.endpoints.first().port = TestPort::freePort();
    NodeDefinition value;
    value.kind = NodeDefinition::Kind::Variable;
    value.nodeId = kValueId;
    value.browseName = QStringLiteral("Value");
    value.initialValue = 7.5;
    value.quality = VariableInfo::Quality::BadWaitingForInitialData;
    NodeDefinition other = value;
    other.nodeId = kOtherId;
    other.browseName = QStringLiteral("Other");
    other.quality = VariableInfo::Quality::Good;
    NodeDefinition object;
    object.kind = NodeDefinition::Kind::Object;
    object.nodeId = QStringLiteral("ns=1;s=Test");
    object.browseName = QStringLiteral("Test");
    object.children = { value, other };
    config.nodes = { object };
    return config;
}

}

class TestClientQuality : public QObject
{
    Q_OBJECT

private slots:
    void clientWriteRefreshesQuality_data();
    void clientWriteRefreshesQuality();
    void unconfiguredVariableRefreshesQuality_data();
    void unconfiguredVariableRefreshesQuality();
};

///
/// \brief A write may change only the status, and its variable need not be selected in the editor.
///
void TestClientQuality::clientWriteRefreshesQuality_data()
{
    QTest::addColumn<double>("value");
    QTest::addColumn<bool>("selected");
    QTest::newRow("changed selected") << 12.0 << true;
    QTest::newRow("same selected") << 7.5 << true;
    QTest::newRow("changed unselected") << 12.0 << false;
    QTest::newRow("same unselected") << 7.5 << false;
}

///
/// \brief Server, client, table and properties agree after a client write without discarding pending edits.
///
void TestClientQuality::clientWriteRefreshesQuality()
{
    QFETCH(double, value);
    QFETCH(bool, selected);
    ServerController controller;
    controller.setProject(project());
    QVERIFY(controller.start());
    MainWindow window;
    window.setController(&controller);
    auto *page = window.findChild<ObjectsPage *>();
    QVERIFY(page);
    QVERIFY(page->selectNode(selected ? kValueId : kOtherId));
    const auto *model = page->variablesModel();
    const int row = model->rowOf(kValueId);
    QVERIFY(row >= 0);
    QModelIndex qualityCell = model->indexForRow(row, VariablesModel::ColQuality);
    QCOMPARE(qualityCell.data().toString(), VariablesModel::qualityText(VariableInfo::Quality::BadWaitingForInitialData));
    auto *indicator = page->findChild<StatusIndicator *>(QStringLiteral("qualityIndicator"));
    auto *name = page->findChild<QLineEdit *>(QStringLiteral("displayNameEdit"));
    auto *initial = page->findChild<QLineEdit *>(QStringLiteral("initialValueEdit"));
    auto *configured = page->findChild<QComboBox *>(QStringLiteral("qualityCombo"));
    QVERIFY(indicator);
    QVERIFY(name);
    QVERIFY(initial);
    QVERIFY(configured);
    name->setText(QStringLiteral("Pending rename"));
    initial->setText(QStringLiteral("123.0"));
    initial->setModified(true);
    configured->setCurrentIndex(configured->findData(quint32(VariableInfo::Quality::Uncertain)));
    QSignalSpy edited(&controller, &ServerController::projectEdited);
    UaTestClient client;
    QCOMPARE(client.connect(controller.server().endpointUrl()), UA_STATUSCODE_GOOD);
    QCOMPARE(client.writeValue(kValueId, value), UA_STATUSCODE_GOOD);
    QVariant read;
    QCOMPARE(client.readValue(kValueId, read), UA_STATUSCODE_GOOD);
    QCOMPARE(read.toDouble(), value);
    QCOMPARE(controller.addressSpace().quality(kValueId), VariableInfo::Quality::Good);
    QTRY_COMPARE(qualityCell.data().toString(), QStringLiteral("Good"));
    QCOMPARE(model->item(row).value.toDouble(), value);
    QCOMPARE(indicator->text(), QStringLiteral("Good"));
    QCOMPARE(indicator->status(), StatusKind::Good);
    QCOMPARE(name->text(), QStringLiteral("Pending rename"));
    QCOMPARE(initial->text(), QStringLiteral("123.0"));
    QVERIFY(initial->isModified());
    QCOMPARE(configured->currentData().toUInt(), quint32(VariableInfo::Quality::Uncertain));
    QCOMPARE(ProjectNodes::find(controller.project().nodes, kValueId)->quality,
             VariableInfo::Quality::BadWaitingForInitialData);
    QCOMPARE(edited.count(), 0);

    QVERIFY(controller.addressSpace().setAccess(kValueId, QStringLiteral("Read")));
    QVERIFY(client.writeValue(kValueId, 99.0) != UA_STATUSCODE_GOOD);
    QCOMPARE(qualityCell.data().toString(), QStringLiteral("Good"));
    QCOMPARE(model->item(row).value.toDouble(), value);
    QVERIFY(controller.addressSpace().setAccess(kValueId, QStringLiteral("Read/Write")));

    if (!selected) {
        QVERIFY(page->selectNode(kValueId));
        qualityCell = model->indexForRow(model->rowOf(kValueId), VariablesModel::ColQuality);
    }
    QCOMPARE(indicator->text(), QStringLiteral("Good"));
    QVERIFY(controller.addressSpace().setValue(kValueId, 7.5));
    QCOMPARE(client.readValue(kValueId, read), UA_STATUSCODE_BADWAITINGFORINITIALDATA);
    QTRY_COMPARE(qualityCell.data().toString(), VariablesModel::qualityText(VariableInfo::Quality::BadWaitingForInitialData));
    QCOMPARE(indicator->status(), StatusKind::Error);
    QCOMPARE(client.writeValue(kValueId, 7.5), UA_STATUSCODE_GOOD);
    QTRY_COMPARE(qualityCell.data().toString(), QStringLiteral("Good"));
    QCOMPARE(indicator->text(), QStringLiteral("Good"));
    auto *revert = page->findChild<QPushButton *>(QStringLiteral("revertButton"));
    QVERIFY(revert);
    revert->click();
    QCOMPARE(indicator->text(), QStringLiteral("Good"));
    QCOMPARE(configured->currentData().toUInt(), quint32(VariableInfo::Quality::BadWaitingForInitialData));
    QCOMPARE(edited.count(), 0);
}

///
/// \brief Imported and type-declared variables must remain observable without simulation settings.
///
void TestClientQuality::unconfiguredVariableRefreshesQuality_data()
{
    QTest::addColumn<bool>("declared");
    QTest::newRow("model variable") << false;
    QTest::newRow("declared variable") << true;
}

///
/// \brief Client writes update unconfigured variables and keep one subscription after a project rebuild.
///
void TestClientQuality::unconfiguredVariableRefreshesQuality()
{
    QFETCH(bool, declared);
    auto config = project();
    config.nodeSetFiles = { QStringLiteral(OUASIM_TEST_DATA_DIR "/pump.NodeSet2.xml") };
    if (declared) {
        NodeDefinition pump;
        pump.kind = NodeDefinition::Kind::Object;
        pump.nodeId = QStringLiteral("ns=1;s=Pump1");
        pump.browseName = QStringLiteral("Pump1");
        pump.typeDefinition = QStringLiteral("nsu=urn:openuasim:test:pump;i=1000");
        config.nodes.append(pump);
    }
    ServerController controller;
    controller.setProject(config);
    QVERIFY(controller.start());
    const int ns = controller.server().uaServer()->namespaces().indexOf(QStringLiteral("urn:openuasim:test:pump"));
    QVERIFY(ns >= 0);
    const QString id = declared ? QStringLiteral("ns=1;s=Pump1.Speed") : QStringLiteral("ns=%1;i=4001").arg(ns);
    auto *variable = controller.server().uaServer()->nodeById<QUaBaseDataVariable>(QUaNodeId(id));
    QVERIFY(variable);
    QVERIFY(!controller.addressSpace().hasVariable(id));
    variable->setStatusCode(QUaStatusCode(UA_StatusCode(UA_STATUSCODE_BADWAITINGFORINITIALDATA)));
    MainWindow window;
    window.setController(&controller);
    auto *page = window.findChild<ObjectsPage *>();
    QVERIFY(page);
    QVERIFY(page->selectNode(id));
    auto *indicator = page->findChild<StatusIndicator *>(QStringLiteral("qualityIndicator"));
    QVERIFY(indicator);
    QCOMPARE(indicator->status(), StatusKind::Error);
    QSignalSpy qualities(&controller, &ServerController::variableQualityChanged);
    UaTestClient client;
    QCOMPARE(client.connect(controller.server().endpointUrl()), UA_STATUSCODE_GOOD);
    QCOMPARE(client.writeValue(id, 9.5), UA_STATUSCODE_GOOD);
    QTRY_COMPARE(indicator->text(), QStringLiteral("Good"));
    const auto *model = page->variablesModel();
    const int row = model->rowOf(id);
    QVERIFY(row >= 0);
    QCOMPARE(model->item(row).quality, VariableInfo::Quality::Good);
    QCOMPARE(model->item(row).value.toDouble(), 9.5);
    QCOMPARE(qualities.count(), 1);
    QVERIFY(controller.project().modelVariables.isEmpty());
    QVERIFY(!controller.addressSpace().hasVariable(id));

    controller.setProject(config);
    variable = controller.server().uaServer()->nodeById<QUaBaseDataVariable>(QUaNodeId(id));
    QVERIFY(variable);
    variable->setStatusCode(QUaStatusCode(UA_StatusCode(UA_STATUSCODE_BADWAITINGFORINITIALDATA)));
    QCOMPARE(client.writeValue(id, 9.5), UA_STATUSCODE_GOOD);
    QTRY_COMPARE(indicator->text(), QStringLiteral("Good"));
    QCOMPARE(qualities.count(), 3);
    QVERIFY(controller.project().modelVariables.isEmpty());
}

QTEST_MAIN(TestClientQuality)

#include "test_clientquality.moc"
