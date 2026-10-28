// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QSignalSpy>
#include <QTest>

#include <QUaServer>

#include "project/methodstatuses.h"
#include "server/modelmethods.h"
#include "server/servercontroller.h"
#include "server/simserver.h"
#include "testport.h"
#include "testproject.h"
#include "uatestclient.h"

Q_DECLARE_METATYPE(MethodInfo)

namespace {

const QString kMethodsUri = QStringLiteral("urn:openuasim:test:methods");
constexpr quint32 kBadInvalidState = 0x80AF0000;

QString testData(const QString &fileName)
{
    return QStringLiteral(OUASIM_TEST_DATA_DIR "/") + fileName;
}

ProjectConfig methodsProject()
{
    ProjectConfig config = TestProject::create();
    config.endpoints.first().port = TestPort::freePort();
    config.nodeSetFiles = { testData(QStringLiteral("methods.NodeSet2.xml")) };
    return config;
}

QString serverId(const QStringList &namespaces, const QString &identifier)
{
    return QStringLiteral("ns=%1;%2").arg(namespaces.indexOf(kMethodsUri)).arg(identifier);
}

bool logContains(const QList<LogEntry> &log, const QString &text)
{
    for (const LogEntry &entry : log) {
        if (entry.message.contains(text))
            return true;
    }
    return false;
}

MethodResponse response(quint32 statusCode, const QStringList &outputs)
{
    MethodResponse result;
    result.statusCode = statusCode;
    result.outputs = outputs;
    return result;
}

}

class TestModelMethods : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void statusNamesRoundTrip();
    void describesModelMethods();
    void describesStandardMethods();
    void parsesOutputTexts();
    void reportsOutputTextsOfWrongType();
    void refusesTextForUntypedOutputs();
    void findsStoredResponses();
    void resolvesStoredResponses();
    void reportsUnresolvableResponses();
    void unconfiguredMethodsAnswerGood();
    void configuredResponseReachesClients();
    void updateModelMethodRejectsInvalidOutput();
    void updateModelMethodRejectsOtherNodes();
    void defaultResponseDropsStoredOne();
    void responsesApplyWhenProjectLoads();
    void removeNodeSetDropsResponses();
    void snapshotDescribesMethods();

private:
    QString id(const QString &identifier) const;

    SimServer _server;
    QUaServer *_ua = nullptr;
};

void TestModelMethods::initTestCase()
{
    _ua = _server.uaServer();
    QVERIFY(_ua->loadNodeSet(testData(QStringLiteral("methods.NodeSet2.xml"))).isOk());
}

QString TestModelMethods::id(const QString &identifier) const
{
    return serverId(_ua->namespaces(), identifier);
}

void TestModelMethods::statusNamesRoundTrip()
{
    QCOMPARE(MethodStatuses::codes().first(), 0u);
    for (quint32 code : MethodStatuses::codes()) {
        quint32 parsed = 1;
        QVERIFY(MethodStatuses::fromName(MethodStatuses::name(code), parsed));
        QCOMPARE(parsed, code);
    }
    QCOMPARE(MethodStatuses::name(kBadInvalidState), QStringLiteral("BadInvalidState"));
    QVERIFY(MethodStatuses::name(0x12345678).isEmpty());
    quint32 unknown = 7;
    QVERIFY(!MethodStatuses::fromName(QStringLiteral("BadSomething"), unknown));
    QCOMPARE(unknown, 7u);
}

///
/// \brief Subtypes of built-in types are typed as the built-in type, enumerations as Int32.
///
void TestModelMethods::describesModelMethods()
{
    const MethodInfo dose = ModelMethods::describe(*_ua, id(QStringLiteral("i=5001")));

    QCOMPARE(dose.kind, MethodInfo::Kind::Model);
    QCOMPARE(dose.displayName, QStringLiteral("Dose"));
    QCOMPARE(dose.inputs.size(), 1);
    QCOMPARE(dose.inputs.first().name, QStringLiteral("Volume"));
    QCOMPARE(dose.inputs.first().dataType, QStringLiteral("Double"));
    QCOMPARE(dose.inputs.first().description, QStringLiteral("Volume to dispense, l"));
    QCOMPARE(dose.outputs.size(), 4);
    QCOMPARE(dose.outputs.at(0).dataType, QStringLiteral("Duration"));
    QCOMPARE(dose.outputs.at(0).valueType, QStringLiteral("Double"));
    QCOMPARE(dose.outputs.at(1).valueType, QStringLiteral("String"));
    QCOMPARE(dose.outputs.at(2).dataType, QStringLiteral("Valve"));
    QCOMPARE(dose.outputs.at(2).valueType, QStringLiteral("Int32"));
    QCOMPARE(dose.outputs.at(3).dataType, QStringLiteral("UInt32[]"));
    QVERIFY(dose.outputs.at(3).array);
    QVERIFY(!dose.outputs.at(0).array);

    const MethodInfo reset = ModelMethods::describe(*_ua, id(QStringLiteral("i=5010")));
    QVERIFY(reset.inputs.isEmpty());
    QVERIFY(reset.outputs.isEmpty());
}

void TestModelMethods::describesStandardMethods()
{
    const MethodInfo method = ModelMethods::describe(*_ua, QStringLiteral("i=11492"));

    QCOMPARE(method.kind, MethodInfo::Kind::Standard);
    QCOMPARE(method.displayName, QStringLiteral("GetMonitoredItems"));
    QCOMPARE(method.inputs.size(), 1);
    QCOMPARE(method.inputs.first().dataType, QStringLiteral("UInt32"));
}

void TestModelMethods::parsesOutputTexts()
{
    const QList<ArgumentInfo> outputs = ModelMethods::describe(*_ua, id(QStringLiteral("i=5001"))).outputs;
    QString problem;

    const QVariantList values = ModelMethods::outputValues(
        outputs, response(0, { QStringLiteral("1.5"), QStringLiteral(" done"), QString(), QStringLiteral("4; 5") }),
        &problem);

    QVERIFY(problem.isEmpty());
    QCOMPARE(values.size(), 4);
    QCOMPARE(values.at(0), QVariant(1.5));
    QCOMPARE(values.at(1), QVariant(QStringLiteral(" done")));
    QVERIFY(!values.at(2).isValid());
    QCOMPARE(values.at(3).toList(), (QVariantList{ 4u, 5u }));
    QCOMPARE(ModelMethods::outputValues(outputs, MethodResponse()).size(), 4);
}

void TestModelMethods::reportsOutputTextsOfWrongType()
{
    const QList<ArgumentInfo> outputs = ModelMethods::describe(*_ua, id(QStringLiteral("i=5001"))).outputs;
    QString problem;

    QVERIFY(ModelMethods::outputValues(outputs, response(0, { QStringLiteral("soon") }), &problem).isEmpty());
    QCOMPARE(problem, QStringLiteral("'soon' is not a Duration value for 'Elapsed'"));

    QVERIFY(ModelMethods::outputValues(outputs, response(0, { {}, {}, {}, QStringLiteral("4; -1") }), &problem)
                .isEmpty());
    QVERIFY(problem.contains(QStringLiteral("'Counts'")));
}

void TestModelMethods::refusesTextForUntypedOutputs()
{
    ArgumentInfo structure;
    structure.name = QStringLiteral("Reading");
    structure.dataType = QStringLiteral("Reading");
    QString problem;

    QCOMPARE(ModelMethods::outputValues({ structure }, MethodResponse(), &problem).size(), 1);
    QVERIFY(ModelMethods::outputValues({ structure }, response(0, { QStringLiteral("1") }), &problem).isEmpty());
    QVERIFY(!problem.isEmpty());
}

void TestModelMethods::findsStoredResponses()
{
    const QList<ModelMethod> methods = { { kMethodsUri, QStringLiteral("i=5001"), {} },
                                         { QStringLiteral("urn:other"), QStringLiteral("i=5010"), {} } };

    QCOMPARE(ModelMethods::indexOf(methods, kMethodsUri, QStringLiteral("i=5001")), 0);
    QCOMPARE(ModelMethods::indexOf(methods, QStringLiteral("urn:other"), QStringLiteral("i=5010")), 1);
    QCOMPARE(ModelMethods::indexOf(methods, kMethodsUri, QStringLiteral("i=5010")), -1);
}

void TestModelMethods::resolvesStoredResponses()
{
    const MethodResponse failing = response(kBadInvalidState, { QStringLiteral("2") });

    QStringList problems;
    const QHash<QString, MethodResponse> resolved =
        ModelMethods::resolve({ { kMethodsUri, QStringLiteral("i=5001"), failing } }, *_ua, &problems);

    QVERIFY(problems.isEmpty());
    QCOMPARE(resolved.size(), 1);
    QVERIFY(resolved.value(id(QStringLiteral("i=5001"))) == failing);
}

void TestModelMethods::reportsUnresolvableResponses()
{
    const QList<ModelMethod> methods = {
        { QStringLiteral("urn:not:loaded"), QStringLiteral("i=5001"), {} },
        { kMethodsUri, QStringLiteral("i=9999"), {} },
        { kMethodsUri, QStringLiteral("i=5001"), response(0, { QStringLiteral("soon") }) },
    };

    QStringList problems;
    QVERIFY(ModelMethods::resolve(methods, *_ua, &problems).isEmpty());

    QCOMPARE(problems.size(), 3);
    QVERIFY(problems.at(0).contains(QStringLiteral("no imported model has the namespace")));
    QVERIFY(problems.at(1).contains(QStringLiteral("the model has no such method")));
    QVERIFY(problems.at(2).contains(QStringLiteral("'soon'")));
}

void TestModelMethods::unconfiguredMethodsAnswerGood()
{
    ServerController controller;
    controller.setProject(methodsProject());
    QVERIFY(controller.start());
    const QStringList namespaces = controller.server().uaServer()->namespaces();
    UaTestClient client;
    QCOMPARE(client.connect(controller.server().endpointUrl()), UA_STATUSCODE_GOOD);
    QVariantList outputs;

    QCOMPARE(client.callMethod(serverId(namespaces, QStringLiteral("i=5000")),
                               serverId(namespaces, QStringLiteral("i=5001")), { 2.5 }, outputs),
             UA_STATUSCODE_GOOD);

    QCOMPARE(outputs.size(), 4);
    QCOMPARE(outputs.at(0), QVariant(0.0));
    QCOMPARE(outputs.at(1).toString(), QString());
    QCOMPARE(outputs.at(2), QVariant(qint32(0)));
    QVERIFY(outputs.at(3).toList().isEmpty());
    QTRY_VERIFY(logContains(controller.log(), QStringLiteral("answered Good")));
}

void TestModelMethods::configuredResponseReachesClients()
{
    ServerController controller;
    controller.setProject(methodsProject());
    QVERIFY(controller.start());
    const QStringList namespaces = controller.server().uaServer()->namespaces();
    const QString doseId = serverId(namespaces, QStringLiteral("i=5001"));
    const QString stationId = serverId(namespaces, QStringLiteral("i=5000"));
    QSignalSpy edited(&controller, &ServerController::projectEdited);
    QSignalSpy changed(&controller, &ServerController::methodChanged);
    QSignalSpy rebuilt(&controller, &ServerController::projectChanged);
    MethodInfo method;
    method.nodeId = doseId;
    method.response = response(0, { QStringLiteral("1.5"), QStringLiteral("done"), QStringLiteral("1"),
                                    QStringLiteral("4; 5") });

    QString error;
    QVERIFY2(controller.updateModelMethod(method, &error), qPrintable(error));

    QCOMPARE(edited.count(), 1);
    QCOMPARE(changed.count(), 1);
    QCOMPARE(changed.first().first().value<MethodInfo>().response.outputs, method.response.outputs);
    QCOMPARE(rebuilt.count(), 0);
    QCOMPARE(controller.project().modelMethods.size(), 1);
    QCOMPARE(controller.project().modelMethods.first().identifier, QStringLiteral("i=5001"));
    UaTestClient client;
    QCOMPARE(client.connect(controller.server().endpointUrl()), UA_STATUSCODE_GOOD);
    QVariantList outputs;
    QCOMPARE(client.callMethod(stationId, doseId, { 2.5 }, outputs), UA_STATUSCODE_GOOD);
    QCOMPARE(outputs.at(0), QVariant(1.5));
    QCOMPARE(outputs.at(1), QVariant(QStringLiteral("done")));
    QCOMPARE(outputs.at(2), QVariant(qint32(1)));
    QCOMPARE(outputs.at(3).toList(), (QVariantList{ 4u, 5u }));

    method.response.statusCode = kBadInvalidState;
    QVERIFY(controller.updateModelMethod(method));
    QCOMPARE(client.callMethod(stationId, doseId, { 2.5 }, outputs), kBadInvalidState);
    QTRY_VERIFY(logContains(controller.log(), QStringLiteral("answered BadInvalidState")));
}

void TestModelMethods::updateModelMethodRejectsInvalidOutput()
{
    ServerController controller;
    controller.setProject(methodsProject());
    MethodInfo method;
    method.nodeId = serverId(controller.server().uaServer()->namespaces(), QStringLiteral("i=5001"));
    method.response = response(0, { QStringLiteral("soon") });
    QSignalSpy edited(&controller, &ServerController::projectEdited);

    QString error;
    QVERIFY(!controller.updateModelMethod(method, &error));

    QCOMPARE(error, QStringLiteral("'soon' is not a Duration value for 'Elapsed'"));
    QCOMPARE(edited.count(), 0);
    QVERIFY(controller.project().modelMethods.isEmpty());
}

void TestModelMethods::updateModelMethodRejectsOtherNodes()
{
    ServerController controller;
    controller.setProject(methodsProject());
    const QStringList namespaces = controller.server().uaServer()->namespaces();

    for (const QString &nodeId : { serverId(namespaces, QStringLiteral("i=5000")), QStringLiteral("i=11492"),
                                   QStringLiteral("ns=1;s=Tank_01") }) {
        MethodInfo method;
        method.nodeId = nodeId;
        QString error;
        QVERIFY2(!controller.updateModelMethod(method, &error), qPrintable(nodeId));
        QCOMPARE(error, QStringLiteral("'%1' is not a method of an imported model").arg(nodeId));
    }
}

void TestModelMethods::defaultResponseDropsStoredOne()
{
    ServerController controller;
    controller.setProject(methodsProject());
    MethodInfo method;
    method.nodeId = serverId(controller.server().uaServer()->namespaces(), QStringLiteral("i=5001"));
    method.response = response(kBadInvalidState, {});
    QVERIFY(controller.updateModelMethod(method));
    QCOMPARE(controller.project().modelMethods.size(), 1);

    method.response = response(0, { QString(), QStringLiteral("  ") });
    QSignalSpy edited(&controller, &ServerController::projectEdited);
    QVERIFY(controller.updateModelMethod(method));

    QVERIFY(controller.project().modelMethods.isEmpty());
    QCOMPARE(edited.count(), 1);
    QVERIFY(controller.updateModelMethod(method));
    QCOMPARE(edited.count(), 1);
}

void TestModelMethods::responsesApplyWhenProjectLoads()
{
    ProjectConfig project = methodsProject();
    project.modelMethods = { { kMethodsUri, QStringLiteral("i=5010"), response(kBadInvalidState, {}) },
                             { kMethodsUri, QStringLiteral("i=5001"), response(0, { QStringLiteral("soon") }) } };
    ServerController controller;
    controller.setProject(project);
    QVERIFY(controller.start());
    const QStringList namespaces = controller.server().uaServer()->namespaces();
    UaTestClient client;
    QCOMPARE(client.connect(controller.server().endpointUrl()), UA_STATUSCODE_GOOD);
    QVariantList outputs;

    QCOMPARE(client.callMethod(serverId(namespaces, QStringLiteral("i=5000")),
                               serverId(namespaces, QStringLiteral("i=5010")), {}, outputs),
             kBadInvalidState);
    QCOMPARE(client.callMethod(serverId(namespaces, QStringLiteral("i=5000")),
                               serverId(namespaces, QStringLiteral("i=5001")), { 1.0 }, outputs),
             UA_STATUSCODE_GOOD);
    QVERIFY(logContains(controller.log(), QStringLiteral("Model method response skipped")));
}

void TestModelMethods::removeNodeSetDropsResponses()
{
    ProjectConfig project = methodsProject();
    project.modelMethods = { { kMethodsUri, QStringLiteral("i=5010"), response(kBadInvalidState, {}) } };
    ServerController controller;
    controller.setProject(project);

    QVERIFY(controller.removeNodeSet(project.nodeSetFiles.first()));

    QVERIFY(controller.project().modelMethods.isEmpty());
}

void TestModelMethods::snapshotDescribesMethods()
{
    ProjectConfig project = methodsProject();
    project.modelMethods = { { kMethodsUri, QStringLiteral("i=5010"), response(kBadInvalidState, {}) } };
    ServerController controller;
    controller.setProject(project);
    NodeDefinition reset;
    reset.kind = NodeDefinition::Kind::Method;
    reset.browseName = QStringLiteral("ResetTank");
    reset.method = NodeDefinition::MethodAction::SetValue;
    const QString resetId = controller.addNode(QStringLiteral("ns=1;s=Tank_01"), reset);
    QVERIFY(!resetId.isEmpty());
    const QStringList namespaces = controller.server().uaServer()->namespaces();

    const QHash<QString, MethodInfo> methods = controller.snapshot().methods;

    const MethodInfo modelReset = methods.value(serverId(namespaces, QStringLiteral("i=5010")));
    QCOMPARE(modelReset.kind, MethodInfo::Kind::Model);
    QCOMPARE(modelReset.response.statusCode, kBadInvalidState);
    QCOMPARE(methods.value(serverId(namespaces, QStringLiteral("i=5001"))).outputs.size(), 4);
    const MethodInfo projectMethod = methods.value(resetId);
    QCOMPARE(projectMethod.kind, MethodInfo::Kind::Project);
    QCOMPARE(projectMethod.action, NodeDefinition::MethodAction::SetValue);
    QCOMPARE(projectMethod.inputs.size(), 2);
}

QTEST_GUILESS_MAIN(TestModelMethods)

#include "test_modelmethods.moc"
