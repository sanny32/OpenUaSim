// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QJsonArray>
#include <QBuffer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>
#include <QUaServer>

#include "project/projectfile.h"
#include "project/portablenodeid.h"
#include "server/methodscenario.h"
#include "server/modelmethods.h"
#include "server/servercontroller.h"
#include "testport.h"
#include "testproject.h"
#include "uatestclient.h"

namespace {

/// \brief Defines a supported scalar or array argument for a project method.
ArgumentInfo argument(const QString &name, const QString &type, bool array = false)
{
    return { name, type + (array ? QStringLiteral("[]") : QString()), type, array, {} };
}

/// \brief Builds a constant source using the editor's text format.
MethodValueSource constant(const QString &text)
{
    MethodValueSource source;
    source.value = text;
    return source;
}

/// \brief Refers to one client input by its signature position.
MethodValueSource input(int index)
{
    MethodValueSource source;
    source.kind = MethodValueSource::Kind::Input;
    source.inputIndex = index;
    return source;
}

/// \brief Refers to a project variable independently of its namespace index.
MethodValueSource variable(const QString &name)
{
    MethodValueSource source;
    source.kind = MethodValueSource::Kind::Variable;
    source.variableId = QStringLiteral("ns=1;s=") + name;
    return source;
}

/// \brief Builds a small project without background simulation interfering with assignment assertions.
ProjectConfig project()
{
    ProjectConfig config = TestProject::create();
    config.endpoints.first().port = TestPort::freePort();
    NodeDefinition object;
    object.kind = NodeDefinition::Kind::Object;
    object.browseName = QStringLiteral("Machine");
    object.nodeId = QStringLiteral("ns=1;s=Machine");
    for (const QString &name : { QStringLiteral("A"), QStringLiteral("B") }) {
        NodeDefinition node;
        node.kind = NodeDefinition::Kind::Variable;
        node.browseName = name;
        node.nodeId = QStringLiteral("ns=1;s=") + name;
        node.initialValue = 1.0;
        object.children.append(node);
    }
    NodeDefinition method;
    method.kind = NodeDefinition::Kind::Method;
    method.method = NodeDefinition::MethodAction::Scenario;
    method.nodeId = QStringLiteral("ns=1;s=Run");
    method.browseName = QStringLiteral("Run");
    method.inputs = { argument(QStringLiteral("Value"), QStringLiteral("Double")) };
    method.outputs = { argument(QStringLiteral("Echo"), QStringLiteral("Double")),
                       argument(QStringLiteral("Stored"), QStringLiteral("Double")) };
    method.response.sources = { input(0), variable(QStringLiteral("B")) };
    object.children.append(method);
    config.nodes = { object };
    return config;
}

}

class TestMethodScenario : public QObject
{
    Q_OBJECT
private slots:
    void projectMethodExecutesAssignmentsInOrder();
    void importedMethodSourcesReachClients();
    void brokenImportedScenarioReturnsErrorAfterLoad();
    void projectSignatureUpdatesAndSurvivesRebuild();
    void arraysAndSimulationReachClients();
    void arrayAssignmentRejectsWrongLength();
    void invalidSourcesAreRejected_data();
    void invalidSourcesAreRejected();
    void badStatusDoesNotAssign();
    void missingRuntimeVariableReturnsError();
    void scenarioRoundTrips();
    void legacyFormat2ResponsesStillLoad();
    void invalidStoredScenarioIsRejected_data();
    void invalidStoredScenarioIsRejected();
    void portableReferencesSurviveNamespaceChanges();
};

void TestMethodScenario::projectMethodExecutesAssignmentsInOrder()
{
    ServerController controller;
    ProjectConfig config = project();
    auto &method = config.nodes.first().children.last();
    method.response.actions = {
        { true, variable(QStringLiteral("A")).variableId, input(0) },
        { false, QStringLiteral("nsu=missing;s=Missing"), constant(QStringLiteral("bad")) },
        { true, variable(QStringLiteral("B")).variableId, variable(QStringLiteral("A")) },
    };
    controller.setProject(config);
    const MethodInfo described = controller.snapshot().methods.value(QStringLiteral("ns=1;s=Run"));
    QCOMPARE(described.inputs.size(), 1);
    QCOMPARE(described.outputs.size(), 2);
    QVERIFY(controller.start());
    UaTestClient client;
    QCOMPARE(client.connect(controller.server().endpointUrl()), UA_STATUSCODE_GOOD);
    QSignalSpy changes(&controller, &ServerController::valuesChanged);
    QVariantList outputs;
    QCOMPARE(client.callMethod(QStringLiteral("ns=1;s=Machine"), QStringLiteral("ns=1;s=Run"), { 12.5 }, outputs), UA_STATUSCODE_GOOD);
    QCOMPARE(outputs, (QVariantList { 12.5, 12.5 }));
    QVariant value;
    QCOMPARE(client.readValue(QStringLiteral("ns=1;s=A"), value), UA_STATUSCODE_GOOD);
    QCOMPARE(value, QVariant(12.5));
    QTRY_VERIFY(!changes.isEmpty());
    MethodInfo edited = controller.snapshot().methods.value(QStringLiteral("ns=1;s=Run"));
    edited.response.sources[0] = constant(QStringLiteral("99"));
    edited.response.actions.clear();
    QSignalSpy rebuilt(&controller, &ServerController::projectChanged);
    QVERIFY(controller.updateModelMethod(edited));
    QCOMPARE(rebuilt.count(), 0);
    outputs.clear();
    QCOMPARE(client.callMethod(QStringLiteral("ns=1;s=Machine"), edited.nodeId, { 7.0 }, outputs), UA_STATUSCODE_GOOD);
    QCOMPARE(outputs, (QVariantList { 99.0, 12.5 }));
}

void TestMethodScenario::projectSignatureUpdatesAndSurvivesRebuild()
{
    ServerController controller;
    ProjectConfig config = project();
    controller.setProject(config);
    MethodInfo edited = controller.snapshot().methods.value(QStringLiteral("ns=1;s=Run"));
    edited.inputs.clear();
    edited.outputs = { argument(QStringLiteral("Result"), QStringLiteral("String")) };
    edited.response.sources = { constant(QStringLiteral("done")) };
    QString error;
    QVERIFY2(controller.updateModelMethod(edited, &error), qPrintable(error));
    QCOMPARE(controller.snapshot().methods.value(edited.nodeId).outputs.size(), 1);
    QVERIFY(controller.start());
    UaTestClient client;
    QCOMPARE(client.connect(controller.server().endpointUrl()), UA_STATUSCODE_GOOD);
    QVariantList outputs;
    QCOMPARE(client.callMethod(QStringLiteral("ns=1;s=Machine"), edited.nodeId, {}, outputs), UA_STATUSCODE_GOOD);
    QCOMPARE(outputs, QVariantList { QStringLiteral("done") });
    edited.inputs = { argument(QStringLiteral("Duplicate"), QStringLiteral("Double")), argument(QStringLiteral("Duplicate"), QStringLiteral("Double")) };
    QVERIFY(!controller.updateModelMethod(edited, &error));
    QVERIFY(error.contains(QStringLiteral("unique")));
    QVERIFY(controller.removeNode(edited.nodeId));
    QVERIFY(!controller.server().uaServer()->isNodeIdUsed(QUaNodeId(edited.nodeId)));
}

void TestMethodScenario::arraysAndSimulationReachClients()
{
    ServerController controller;
    ProjectConfig config = project();
    auto &method = config.nodes.first().children.last();
    method.inputs = { argument(QStringLiteral("Values"), QStringLiteral("UInt32"), true) };
    method.outputs = { argument(QStringLiteral("Echo"), QStringLiteral("UInt32"), true), argument(QStringLiteral("Sample"), QStringLiteral("Double")) };
    MethodValueSource simulation;
    simulation.kind = MethodValueSource::Kind::Simulation;
    simulation.simulation.function = SimulationFunction::Ramp;
    simulation.simulation.offset = 42.0;
    method.response.sources = { input(0), simulation };
    controller.setProject(config);
    QVERIFY(controller.start());
    UaTestClient client;
    QCOMPARE(client.connect(controller.server().endpointUrl()), UA_STATUSCODE_GOOD);
    QVariantList outputs;
    const QVariantList values { quint32(2), quint32(7) };
    QCOMPARE(client.callMethod(QStringLiteral("ns=1;s=Machine"), QStringLiteral("ns=1;s=Run"), { QVariant(values) }, outputs), UA_STATUSCODE_GOOD);
    QCOMPARE(outputs.first().toList(), values);
    QCOMPARE(outputs.last(), QVariant(42.0));
}

void TestMethodScenario::invalidSourcesAreRejected_data()
{
    QTest::addColumn<int>("which");
    for (int index = 0; index < 7; ++index)
        QTest::newRow(qPrintable(QString::number(index))) << index;
}

void TestMethodScenario::arrayAssignmentRejectsWrongLength()
{
    ServerController controller;
    ProjectConfig config = project();
    auto &target = config.nodes.first().children[1];
    target.dataType = QStringLiteral("UInt32");
    target.arrayLength = 2;
    target.initialValue = QVariantList { quint32(0), quint32(0) };
    auto &method = config.nodes.first().children.last();
    method.inputs = { argument(QStringLiteral("Values"), QStringLiteral("UInt32"), true) };
    method.outputs.clear();
    method.response.sources.clear();
    method.response.actions = { { true, variable(QStringLiteral("A")).variableId, constant(QStringLiteral("9")) },
                                { true, variable(QStringLiteral("B")).variableId, input(0) } };
    controller.setProject(config);
    QVERIFY(controller.start());
    UaTestClient client;
    QCOMPARE(client.connect(controller.server().endpointUrl()), UA_STATUSCODE_GOOD);
    QVariantList outputs;
    const QVariantList wrongLength { quint32(1), quint32(2), quint32(3) };
    QCOMPARE(client.callMethod(QStringLiteral("ns=1;s=Machine"), QStringLiteral("ns=1;s=Run"),
                               { QVariant(wrongLength) }, outputs), UA_STATUSCODE_BADTYPEMISMATCH);
    QVariant value;
    QCOMPARE(client.readValue(QStringLiteral("ns=1;s=A"), value), UA_STATUSCODE_GOOD);
    QCOMPARE(value, QVariant(9.0));
    QCOMPARE(client.readValue(QStringLiteral("ns=1;s=B"), value), UA_STATUSCODE_GOOD);
    QCOMPARE(value.toList(), (QVariantList { quint32(0), quint32(0) }));
    const QVariantList matching { quint32(4), quint32(5) };
    QCOMPARE(client.callMethod(QStringLiteral("ns=1;s=Machine"), QStringLiteral("ns=1;s=Run"),
                               { QVariant(matching) }, outputs), UA_STATUSCODE_GOOD);
    QCOMPARE(client.readValue(QStringLiteral("ns=1;s=B"), value), UA_STATUSCODE_GOOD);
    QCOMPARE(value.toList(), matching);
}

void TestMethodScenario::invalidSourcesAreRejected()
{
    QFETCH(int, which);
    ServerController controller;
    ProjectConfig config = project();
    controller.setProject(config);
    MethodInfo edited = controller.snapshot().methods.value(QStringLiteral("ns=1;s=Run"));
    switch (which) {
    case 0: edited.response.sources[0] = input(9); break;
    case 1: edited.response.sources[0] = constant(QStringLiteral("no number")); break;
    case 2: edited.response.sources[0] = variable(QStringLiteral("Missing")); break;
    case 3: edited.response.actions = { { true, QStringLiteral("i=2258"), constant(QStringLiteral("1")) } }; break;
    case 4: edited.inputs[0] = argument(QStringLiteral("Value"), QStringLiteral("String")); break;
    case 5: edited.response.sources[0].kind = MethodValueSource::Kind::Simulation; break;
    case 6: edited.outputs[0].array = true; break;
    }
    QString error;
    QSignalSpy changed(&controller, &ServerController::projectEdited);
    QVERIFY(!controller.updateModelMethod(edited, &error));
    QVERIFY(!error.isEmpty());
    QCOMPARE(changed.count(), 0);
}

void TestMethodScenario::badStatusDoesNotAssign()
{
    ServerController controller;
    ProjectConfig config = project();
    config.nodes.first().children.last().response.actions = { { true, variable(QStringLiteral("A")).variableId, input(0) } };
    config.nodes.first().children.last().response.statusCode = UA_STATUSCODE_BADINVALIDSTATE;
    controller.setProject(config);
    QVERIFY(controller.start());
    UaTestClient client;
    QCOMPARE(client.connect(controller.server().endpointUrl()), UA_STATUSCODE_GOOD);
    QVariantList outputs;
    QCOMPARE(client.callMethod(QStringLiteral("ns=1;s=Machine"), QStringLiteral("ns=1;s=Run"), { 8.0 }, outputs), UA_STATUSCODE_BADINVALIDSTATE);
    QVariant value;
    QCOMPARE(client.readValue(QStringLiteral("ns=1;s=A"), value), UA_STATUSCODE_GOOD);
    QCOMPARE(value, QVariant(1.0));
}

void TestMethodScenario::missingRuntimeVariableReturnsError()
{
    ServerController controller;
    ProjectConfig config = project();
    config.nodes.first().children.last().response.sources[1] = variable(QStringLiteral("Missing"));
    controller.setProject(config);
    QVERIFY(controller.start());
    UaTestClient client;
    QCOMPARE(client.connect(controller.server().endpointUrl()), UA_STATUSCODE_GOOD);
    QVariantList outputs;
    QCOMPARE(client.callMethod(QStringLiteral("ns=1;s=Machine"), QStringLiteral("ns=1;s=Run"), { 8.0 }, outputs), UA_STATUSCODE_BADCONFIGURATIONERROR);
    QVERIFY(outputs.isEmpty());
}

void TestMethodScenario::scenarioRoundTrips()
{
    ProjectConfig config = project();
    auto &method = config.nodes.first().children.last();
    method.response.actions = { { true, variable(QStringLiteral("A")).variableId, input(0) },
                                { false, QString(), constant(QStringLiteral("ignored")) },
                                { false, QString(), input(-1) } };
    method.response.sources.first().simulation.function = SimulationFunction::Counter;
    method.response.sources.first().simulation.amplitude = 7.0;
    const auto loaded = ProjectFile::fromJson(ProjectFile::toJson(config));
    QVERIFY2(loaded.ok(), qPrintable(loaded.error));
    const NodeDefinition &restored = loaded.project.nodes.first().children.last();
    QCOMPARE(restored.method, NodeDefinition::MethodAction::Scenario);
    QVERIFY(restored.response == method.response);
    QCOMPARE(restored.inputs.first().valueType, QStringLiteral("Double"));
    config.modelMethods = { { QStringLiteral("urn:methods"), QStringLiteral("i=1"), method.response } };
    const auto imported = ProjectFile::fromJson(ProjectFile::toJson(config));
    QVERIFY2(imported.ok(), qPrintable(imported.error));
    QVERIFY(imported.project.modelMethods.first().response == method.response);
}

void TestMethodScenario::invalidStoredScenarioIsRejected_data()
{
    QTest::addColumn<QByteArray>("field");
    QTest::addColumn<QByteArray>("replacement");
    QTest::newRow("source") << QByteArray("\"kind\": \"input\"") << QByteArray("\"kind\": \"unknown\"");
    QTest::newRow("index") << QByteArray("\"inputIndex\": 0") << QByteArray("\"inputIndex\": -1");
    QTest::newRow("type") << QByteArray("\"dataType\": \"Double\"") << QByteArray("\"dataType\": \"Unknown\"");
    QTest::newRow("constant-text") << QByteArray("\"value\": \"\"") << QByteArray("\"value\": false");
}

void TestMethodScenario::legacyFormat2ResponsesStillLoad()
{
    ProjectConfig config = TestProject::create();
    MethodResponse response;
    response.outputs = { QStringLiteral("2.5") };
    config.modelMethods = { { QStringLiteral("urn:methods"), QStringLiteral("i=1"), response } };
    QJsonObject json = QJsonDocument::fromJson(ProjectFile::toJson(config)).object();
    json.insert(QStringLiteral("formatVersion"), 2);
    const auto loaded = ProjectFile::fromJson(QJsonDocument(json).toJson());
    QVERIFY2(loaded.ok(), qPrintable(loaded.error));
    QVERIFY(loaded.project.modelMethods.first().response == response);
    QCOMPARE(QJsonDocument::fromJson(ProjectFile::toJson(loaded.project)).object()
                 .value(QStringLiteral("formatVersion")).toInt(), 3);
}

void TestMethodScenario::invalidStoredScenarioIsRejected()
{
    QFETCH(QByteArray, field);
    QFETCH(QByteArray, replacement);
    QByteArray json = ProjectFile::toJson(project());
    QVERIFY(json.contains(field));
    json.replace(field, replacement);
    const auto loaded = ProjectFile::fromJson(json);
    QVERIFY(!loaded.ok());
    QVERIFY(!loaded.error.isEmpty());
}

void TestMethodScenario::portableReferencesSurviveNamespaceChanges()
{
    QUaServer server;
    QByteArray xml = R"(<UANodeSet xmlns="http://opcfoundation.org/UA/2011/03/UANodeSet.xsd"
        xmlns:uax="http://opcfoundation.org/UA/2008/02/Types.xsd">
      <NamespaceUris><Uri>urn:extra</Uri><Uri>urn:machine</Uri></NamespaceUris>
      <UAVariable NodeId="ns=2;s=Speed" BrowseName="2:Speed" DataType="i=290" ValueRank="-1">
        <DisplayName>Speed</DisplayName><References>
          <Reference ReferenceType="i=35" IsForward="false">i=85</Reference>
          <Reference ReferenceType="i=40">i=63</Reference>
        </References><Value><uax:Double>17.5</uax:Double></Value>
      </UAVariable>
    </UANodeSet>)";
    QBuffer buffer(&xml);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    QVERIFY(server.loadNodeSet(&buffer).isOk());
    QCOMPARE(server.namespaces().indexOf(QStringLiteral("urn:machine")), 3);
    MethodInfo info;
    info.outputs = { argument(QStringLiteral("Value"), QStringLiteral("Double")) };
    MethodValueSource source;
    source.kind = MethodValueSource::Kind::Variable;
    source.variableId = QStringLiteral("nsu=urn:machine;s=Speed");
    info.response.sources = { source };
    info.response.actions = { { true, source.variableId, constant(QStringLiteral("18.5")) } };
    QVariantList outputs;
    QHash<QString, QVariant> changes;
    QString error;
    QCOMPARE(MethodScenario::execute(server, nullptr, info, {}, outputs, changes, error), UA_STATUSCODE_GOOD);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(outputs, QVariantList { 18.5 });
    QCOMPARE(changes.value(QStringLiteral("ns=3;s=Speed")), QVariant(18.5));
    QUaVariableAttributes attributes;
    QVERIFY(server.variableAttributes(QUaNodeId(QStringLiteral("ns=3;s=Speed")), attributes));
    QCOMPARE(attributes.dataType, QUaNodeId(0, quint32(290)));
}

void TestMethodScenario::importedMethodSourcesReachClients()
{
    ServerController controller;
    ProjectConfig config = project();
    config.nodeSetFiles = { QStringLiteral(OUASIM_TEST_DATA_DIR "/methods.NodeSet2.xml") };
    controller.setProject(config);
    const int ns = controller.server().uaServer()->namespaces().indexOf(QStringLiteral("urn:openuasim:test:methods"));
    const QString methodId = QStringLiteral("ns=%1;i=5001").arg(ns);
    MethodInfo info = controller.snapshot().methods.value(methodId);
    info.response.sources = { input(0), constant(QStringLiteral("done")), constant(QStringLiteral("1")), constant(QStringLiteral("4; 5")) };
    info.response.actions = { { true, variable(QStringLiteral("A")).variableId, input(0) } };
    QString error;
    QVERIFY2(controller.updateModelMethod(info, &error), qPrintable(error));
    QVERIFY(controller.start());
    UaTestClient client;
    QCOMPARE(client.connect(controller.server().endpointUrl()), UA_STATUSCODE_GOOD);
    QVariantList outputs;
    QCOMPARE(client.callMethod(QStringLiteral("ns=%1;i=5000").arg(ns), methodId, { 2.5 }, outputs), UA_STATUSCODE_GOOD);
    QCOMPARE(outputs.size(), 4);
    QCOMPARE(outputs.at(0), QVariant(2.5));
    QCOMPARE(outputs.at(1).toString(), QStringLiteral("done"));
    QCOMPARE(outputs.at(2).toInt(), 1);
    QCOMPARE(outputs.at(3).toList(), (QVariantList { quint32(4), quint32(5) }));
    QVariant stored;
    QCOMPARE(client.readValue(QStringLiteral("ns=1;s=A"), stored), UA_STATUSCODE_GOOD);
    QCOMPARE(stored, QVariant(2.5));
}

void TestMethodScenario::brokenImportedScenarioReturnsErrorAfterLoad()
{
    ProjectConfig config = project();
    config.nodeSetFiles = { QStringLiteral(OUASIM_TEST_DATA_DIR "/methods.NodeSet2.xml") };
    MethodResponse response;
    response.sources = { variable(QStringLiteral("Missing")) };
    config.modelMethods = { { QStringLiteral("urn:openuasim:test:methods"), QStringLiteral("i=5001"), response } };
    const auto loaded = ProjectFile::fromJson(ProjectFile::toJson(config));
    QVERIFY2(loaded.ok(), qPrintable(loaded.error));
    ServerController controller;
    controller.setProject(loaded.project);
    QVERIFY(controller.start());
    const int ns = controller.server().uaServer()->namespaces().indexOf(QStringLiteral("urn:openuasim:test:methods"));
    UaTestClient client;
    QCOMPARE(client.connect(controller.server().endpointUrl()), UA_STATUSCODE_GOOD);
    QVariantList outputs;
    QCOMPARE(client.callMethod(QStringLiteral("ns=%1;i=5000").arg(ns), QStringLiteral("ns=%1;i=5001").arg(ns),
                               { 1.0 }, outputs), UA_STATUSCODE_BADCONFIGURATIONERROR);
}

QTEST_GUILESS_MAIN(TestMethodScenario)
#include "test_methodscenario.moc"
