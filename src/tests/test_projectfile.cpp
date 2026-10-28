// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QSysInfo>
#include <QTest>
#include <QTimeZone>

#include "project/defaultproject.h"
#include "project/projectfile.h"

namespace {

QByteArray projectWithNode(const QJsonObject &node)
{
    const QJsonObject root {
        { QStringLiteral("formatVersion"), 1 },
        { QStringLiteral("nodes"), QJsonArray { node } },
    };
    return QJsonDocument(root).toJson();
}

QJsonObject variableNode(const QString &key, const QJsonValue &value)
{
    QJsonObject node {
        { QStringLiteral("kind"), QStringLiteral("variable") },
        { QStringLiteral("browseName"), QStringLiteral("Value") },
    };
    node.insert(key, value);
    return node;
}

const NodeDefinition &firstVariable(const ProjectConfig &project)
{
    return project.nodes.first().children.first().children.first();
}

QByteArray projectWithModelVariables(const QJsonArray &variables)
{
    const QJsonObject root {
        { QStringLiteral("formatVersion"), ProjectFile::kFormatVersion },
        { QStringLiteral("modelVariables"), variables },
    };
    return QJsonDocument(root).toJson();
}

QByteArray projectWithModelMethods(const QJsonArray &methods)
{
    const QJsonObject root {
        { QStringLiteral("formatVersion"), ProjectFile::kFormatVersion },
        { QStringLiteral("modelMethods"), methods },
    };
    return QJsonDocument(root).toJson();
}

QJsonObject modelMethod(const QString &namespaceUri, const QString &nodeId, const QString &status)
{
    return {
        { QStringLiteral("namespaceUri"), namespaceUri },
        { QStringLiteral("nodeId"), nodeId },
        { QStringLiteral("status"), status },
    };
}

QJsonObject modelVariable(const QString &namespaceUri, const QString &nodeId)
{
    return {
        { QStringLiteral("namespaceUri"), namespaceUri },
        { QStringLiteral("kind"), QStringLiteral("variable") },
        { QStringLiteral("browseName"), QStringLiteral("Level") },
        { QStringLiteral("nodeId"), nodeId },
    };
}

}

class TestProjectFile : public QObject
{
    Q_OBJECT

private slots:
    void defaultProjectRoundTrips();
    void loadsLegacyAccess_data();
    void loadsLegacyAccess();
    void preservesVariableFields_data();
    void preservesVariableFields();
    void preservesEveryDataTypeValue_data();
    void preservesEveryDataTypeValue();
    void preservesQuality();
    void preservesHistorizingAndHistoryLimits();
    void preservesDiscoveryRegistration();
    void preservesEveryQuality();
    void preservesSimulationValues();
    void preservesMethods();
    void preservesAlarms();
    void preservesUsers();
    void movesFormat1EndpointLoginsToUsers();
    void emptyProjectRoundTrips();
    void missingFieldsUseDefaults();
    void preservesExplicitHostname_data();
    void preservesExplicitHostname();
    void marksMachineHostname();
    void machineHostnameFollowsLoadingComputer();
    void keepsTrustedClientsOnlyKey();
    void keepsUserCertificateFlag();
    void rejectsMalformedJson();
    void rejectsNonObjectRoot();
    void rejectsMissingVersion();
    void rejectsNewerVersion();
    void rejectsInvalidNode_data();
    void rejectsInvalidNode();
    void rejectsDuplicateNodeIds();
    void savesAndLoadsFile();
    void preservesModelVariables();
    void rejectsInvalidModelVariable_data();
    void rejectsInvalidModelVariable();
    void preservesModelMethods();
    void modelMethodWithoutStatusAnswersGood();
    void rejectsInvalidModelMethod_data();
    void rejectsInvalidModelMethod();
    void storesModelPathsRelativeToFile();
    void loadReportsMissingFile();
    void saveReportsUnwritablePath();
};

void TestProjectFile::loadsLegacyAccess_data()
{
    QTest::addColumn<QString>("stored");
    QTest::addColumn<QString>("canonical");
    QTest::newRow("read") << QStringLiteral("\u0427\u0442\u0435\u043d\u0438\u0435") << QStringLiteral("Read");
    QTest::newRow("write") << QStringLiteral("\u0417\u0430\u043f\u0438\u0441\u044c") << QStringLiteral("Write");
    QTest::newRow("readWrite") << QStringLiteral("\u0427\u0442\u0435\u043d\u0438\u0435/\u0437\u0430\u043f\u0438\u0441\u044c") << QStringLiteral("Read/Write");
}

void TestProjectFile::loadsLegacyAccess()
{
    QFETCH(QString, stored);
    QFETCH(QString, canonical);
    const auto loaded = ProjectFile::fromJson(projectWithNode(variableNode(QStringLiteral("access"), stored)));
    QVERIFY2(loaded.ok(), qPrintable(loaded.error));
    QCOMPARE(loaded.project.nodes.first().access, canonical);
    const QJsonObject saved = QJsonDocument::fromJson(ProjectFile::toJson(loaded.project)).object();
    QCOMPARE(saved.value(QStringLiteral("nodes")).toArray().first().toObject()
                 .value(QStringLiteral("access")).toString(), canonical);

    QJsonObject model = modelVariable(QStringLiteral("urn:test"), QStringLiteral("ns=1;s=Level"));
    model.insert(QStringLiteral("access"), stored);
    const auto loadedModel = ProjectFile::fromJson(projectWithModelVariables({model}));
    QVERIFY2(loadedModel.ok(), qPrintable(loadedModel.error));
    QCOMPARE(loadedModel.project.modelVariables.first().variable.access, canonical);
}

void TestProjectFile::defaultProjectRoundTrips()
{
    const ProjectConfig original = DefaultProject::create();
    const ProjectFile::LoadResult result = ProjectFile::fromJson(ProjectFile::toJson(original));
    QVERIFY2(result.ok(), qPrintable(result.error));
    QCOMPARE(ProjectFile::toJson(result.project), ProjectFile::toJson(original));
}

void TestProjectFile::preservesVariableFields_data()
{
    QTest::addColumn<QString>("icon");
    for (const char *name : { "signal", "image", "chart", "network", "database" })
        QTest::newRow(name) << QString::fromLatin1(name);
}

void TestProjectFile::preservesVariableFields()
{
    QFETCH(QString, icon);
    ProjectConfig original = DefaultProject::create();
    original.nodes.first().children.first().children.first().icon = icon;
    const ProjectConfig loaded = ProjectFile::fromJson(ProjectFile::toJson(original)).project;

    const NodeDefinition &speed = firstVariable(loaded);
    QCOMPARE(speed.kind, NodeDefinition::Kind::Variable);
    QCOMPARE(speed.browseName, QStringLiteral("Speed"));
    QCOMPARE(speed.nodeId, QStringLiteral("ns=1;s=Motor_01.Speed"));
    QCOMPARE(speed.icon, icon);
    QCOMPARE(speed.dataType, QStringLiteral("Double"));
    QCOMPARE(speed.access, QStringLiteral("Read/Write"));
    QCOMPARE(speed.initialValue, QVariant(1000.0));
    QCOMPARE(speed.simulation.function, SimulationFunction::Sine);
    QCOMPARE(speed.simulation.intervalMs, 100);
    QCOMPARE(speed.simulation.periodMs, 10000);
    QCOMPARE(speed.simulation.amplitude, 500.0);
    QCOMPARE(speed.simulation.offset, 1000.0);
    QCOMPARE(loaded.endpoints.size(), original.endpoints.size());
    for (int i = 0; i < original.endpoints.size(); ++i) {
        QCOMPARE(loaded.endpoints.at(i).url(), original.endpoints.at(i).url());
        QCOMPARE(loaded.endpoints.at(i).securityPolicy, original.endpoints.at(i).securityPolicy);
        QCOMPARE(loaded.endpoints.at(i).securityMode, original.endpoints.at(i).securityMode);
    }
}

void TestProjectFile::preservesEveryDataTypeValue_data()
{
    QTest::addColumn<QString>("dataType");
    QTest::addColumn<QVariant>("value");

    QTest::newRow("Boolean") << "Boolean" << QVariant(true);
    QTest::newRow("SByte") << "SByte" << QVariant::fromValue(qint8(-7));
    QTest::newRow("UInt32") << "UInt32" << QVariant::fromValue(quint32(4000000000u));
    QTest::newRow("Int64") << "Int64" << QVariant::fromValue(qint64(-123456789));
    QTest::newRow("Float") << "Float" << QVariant::fromValue(2.5f);
    QTest::newRow("String") << "String" << QVariant(QStringLiteral("hello"));
    QTest::newRow("DateTime") << "DateTime"
                              << QVariant(QDateTime(QDate(2026, 1, 2), QTime(3, 4, 5, 6), QTimeZone::UTC));
}

void TestProjectFile::preservesEveryDataTypeValue()
{
    QFETCH(QString, dataType);
    QFETCH(QVariant, value);

    NodeDefinition node;
    node.kind = NodeDefinition::Kind::Variable;
    node.browseName = QStringLiteral("Value");
    node.dataType = dataType;
    node.initialValue = value;
    ProjectConfig project;
    project.nodes.append(node);

    const ProjectFile::LoadResult result = ProjectFile::fromJson(ProjectFile::toJson(project));
    QVERIFY2(result.ok(), qPrintable(result.error));
    const QVariant loaded = result.project.nodes.first().initialValue;
    QCOMPARE(loaded.metaType(), value.metaType());
    QCOMPARE(loaded, value);
}

void TestProjectFile::preservesQuality()
{
    NodeDefinition node;
    node.kind = NodeDefinition::Kind::Variable;
    node.browseName = QStringLiteral("Value");
    node.quality = VariableInfo::Quality::Uncertain;
    ProjectConfig project;
    project.nodes.append(node);

    const QByteArray json = ProjectFile::toJson(project);
    QVERIFY(json.contains("\"uncertain\""));
    const ProjectFile::LoadResult result = ProjectFile::fromJson(json);
    QVERIFY2(result.ok(), qPrintable(result.error));
    QCOMPARE(result.project.nodes.first().quality, VariableInfo::Quality::Uncertain);
}

void TestProjectFile::preservesEveryQuality()
{
    ProjectConfig project;
    for (VariableInfo::Quality quality : VariableInfo::qualities()) {
        NodeDefinition node;
        node.kind = NodeDefinition::Kind::Variable;
        node.browseName = QStringLiteral("Value%1").arg(project.nodes.size());
        node.quality = quality;
        project.nodes.append(node);
    }

    const QByteArray json = ProjectFile::toJson(project);
    QVERIFY(json.contains("\"bad_sensor_failure\""));
    const ProjectFile::LoadResult result = ProjectFile::fromJson(json);
    QVERIFY2(result.ok(), qPrintable(result.error));
    QCOMPARE(result.project.nodes.size(), project.nodes.size());
    for (qsizetype index = 0; index < project.nodes.size(); ++index)
        QCOMPARE(result.project.nodes.at(index).quality, project.nodes.at(index).quality);
}

void TestProjectFile::preservesSimulationValues()
{
    NodeDefinition node;
    node.kind = NodeDefinition::Kind::Variable;
    node.browseName = QStringLiteral("State");
    node.dataType = QStringLiteral("String");
    node.simulation.function = SimulationFunction::List;
    node.simulation.values = { QStringLiteral("idle"), QStringLiteral("run") };
    ProjectConfig project;
    project.nodes.append(node);

    project.nodes.first().arrayLength = 5;

    const ProjectFile::LoadResult result = ProjectFile::fromJson(ProjectFile::toJson(project));
    QVERIFY2(result.ok(), qPrintable(result.error));
    QCOMPARE(result.project.nodes.first().arrayLength, 5);
    QCOMPARE(result.project.nodes.first().simulation.function, SimulationFunction::List);
    QCOMPARE(result.project.nodes.first().simulation.values, node.simulation.values);
}

void TestProjectFile::preservesMethods()
{
    NodeDefinition method;
    method.kind = NodeDefinition::Kind::Method;
    method.browseName = QStringLiteral("SetValue");
    method.method = NodeDefinition::MethodAction::SetValue;
    NodeDefinition object;
    object.kind = NodeDefinition::Kind::Object;
    object.browseName = QStringLiteral("Motor");
    object.children = { method };
    ProjectConfig project;
    project.nodes = { object };

    const ProjectFile::LoadResult result = ProjectFile::fromJson(ProjectFile::toJson(project));
    QVERIFY2(result.ok(), qPrintable(result.error));
    const NodeDefinition &loaded = result.project.nodes.first().children.first();
    QCOMPARE(loaded.kind, NodeDefinition::Kind::Method);
    QCOMPARE(loaded.method, NodeDefinition::MethodAction::SetValue);
}

void TestProjectFile::preservesAlarms()
{
    NodeDefinition level;
    level.kind = NodeDefinition::Kind::Variable;
    level.browseName = QStringLiteral("Level");
    level.alarm.enabled = true;
    level.alarm.highEnabled = true;
    level.alarm.high = 80.0;
    level.alarm.lowLowEnabled = true;
    level.alarm.lowLow = 5.0;
    level.alarm.severity = 700;
    NodeDefinition plain = level;
    plain.browseName = QStringLiteral("Plain");
    plain.alarm = AlarmSettings();
    ProjectConfig project;
    project.nodes = { level, plain };

    const QByteArray json = ProjectFile::toJson(project);
    const ProjectFile::LoadResult result = ProjectFile::fromJson(json);
    QVERIFY2(result.ok(), qPrintable(result.error));
    QVERIFY(result.project.nodes.at(0).alarm == level.alarm);
    QVERIFY(!result.project.nodes.at(1).alarm.enabled);
    QCOMPARE(json.count("\"alarm\""), 1);
}

void TestProjectFile::preservesUsers()
{
    ProjectConfig project;
    project.users = { { QStringLiteral("operator"), QStringLiteral("secret") },
                      { QStringLiteral("viewer"), QString() } };
    const ProjectFile::LoadResult result = ProjectFile::fromJson(ProjectFile::toJson(project));
    QVERIFY2(result.ok(), qPrintable(result.error));
    QCOMPARE(result.project.users.size(), 2);
    QCOMPARE(result.project.users.at(0).username, QStringLiteral("operator"));
    QCOMPARE(result.project.users.at(0).password, QStringLiteral("secret"));
    QCOMPARE(result.project.users.at(1).username, QStringLiteral("viewer"));
}

void TestProjectFile::movesFormat1EndpointLoginsToUsers()
{
    const auto login = [](const QString &name, const QString &password) {
        return QJsonObject { { QStringLiteral("usernamePassword"), true },
                             { QStringLiteral("username"), name },
                             { QStringLiteral("password"), password } };
    };
    const QJsonObject root {
        { QStringLiteral("formatVersion"), 1 },
        { QStringLiteral("endpoints"), QJsonArray { login(QStringLiteral("operator"), QStringLiteral("a")),
                                                    login(QStringLiteral("operator"), QStringLiteral("b")),
                                                    login(QString(), QString()) } },
    };
    const ProjectFile::LoadResult result = ProjectFile::fromJson(QJsonDocument(root).toJson());
    QVERIFY2(result.ok(), qPrintable(result.error));
    QCOMPARE(result.project.users.size(), 1);
    QCOMPARE(result.project.users.first().username, QStringLiteral("operator"));
    QCOMPARE(result.project.users.first().password, QStringLiteral("a"));
    QVERIFY(result.project.endpoints.first().usernamePassword);
    const QJsonObject saved = QJsonDocument::fromJson(ProjectFile::toJson(result.project)).object();
    QCOMPARE(saved.value(QStringLiteral("formatVersion")).toInt(), ProjectFile::kFormatVersion);
    QVERIFY(!saved.value(QStringLiteral("endpoints")).toArray().first().toObject().contains(QStringLiteral("username")));
}

void TestProjectFile::preservesHistorizingAndHistoryLimits()
{
    NodeDefinition recorded;
    recorded.kind = NodeDefinition::Kind::Variable;
    recorded.browseName = QStringLiteral("Recorded");
    recorded.historizing = true;
    NodeDefinition plain = recorded;
    plain.browseName = QStringLiteral("Plain");
    plain.historizing = false;
    ProjectConfig project;
    project.nodes = { recorded, plain };
    project.server.history = { 5000, 30 };

    const QByteArray json = ProjectFile::toJson(project);
    QCOMPARE(json.count("\"historizing\""), 1);
    const ProjectFile::LoadResult result = ProjectFile::fromJson(json);
    QVERIFY2(result.ok(), qPrintable(result.error));
    QVERIFY(result.project.nodes.at(0).historizing);
    QVERIFY(!result.project.nodes.at(1).historizing);
    QCOMPARE(result.project.server.history.maxValues, 5000);
    QCOMPARE(result.project.server.history.maxAgeMinutes, 30);
}

void TestProjectFile::preservesDiscoveryRegistration()
{
    ProjectConfig project;
    project.server.discovery = { true, QStringLiteral("opc.tcp://lds.plant:4840"), 120, true };

    const ProjectFile::LoadResult result = ProjectFile::fromJson(ProjectFile::toJson(project));
    QVERIFY2(result.ok(), qPrintable(result.error));
    const DiscoveryRegistration &discovery = result.project.server.discovery;
    QVERIFY(discovery.enabled);
    QCOMPARE(discovery.url, QStringLiteral("opc.tcp://lds.plant:4840"));
    QCOMPARE(discovery.intervalSeconds, 120);
    QVERIFY(discovery.verifyCertificate);
}

void TestProjectFile::emptyProjectRoundTrips()
{
    const ProjectFile::LoadResult result = ProjectFile::fromJson(ProjectFile::toJson(ProjectConfig()));
    QVERIFY(result.ok());
    QVERIFY(result.project.nodes.isEmpty());
    QVERIFY(result.project.endpoints.isEmpty());
    QCOMPARE(result.project.server.applicationName, QStringLiteral("OpenUaSim"));
}

void TestProjectFile::missingFieldsUseDefaults()
{
    const QJsonObject endpoint { { QStringLiteral("name"), QStringLiteral("E") } };
    const QJsonObject root {
        { QStringLiteral("formatVersion"), 1 },
        { QStringLiteral("endpoints"), QJsonArray { endpoint } },
        { QStringLiteral("nodes"), QJsonArray { variableNode(QStringLiteral("icon"), QString()) } },
    };
    const ProjectFile::LoadResult result = ProjectFile::fromJson(QJsonDocument(root).toJson());
    QVERIFY2(result.ok(), qPrintable(result.error));

    const EndpointInfo &loadedEndpoint = result.project.endpoints.first();
    QCOMPARE(loadedEndpoint.hostname, QSysInfo::machineHostName());
    QCOMPARE(loadedEndpoint.port, kDefaultEndpointPort);
    QCOMPARE(loadedEndpoint.protocol, QStringLiteral("opc.tcp"));
    QVERIFY(loadedEndpoint.enabled);
    QVERIFY(loadedEndpoint.allowAnonymous);

    const NodeDefinition &variable = result.project.nodes.first();
    QCOMPARE(variable.dataType, QStringLiteral("Double"));
    QCOMPARE(variable.access, QStringLiteral("Read/Write"));
    QCOMPARE(variable.initialValue, QVariant(0.0));
    QCOMPARE(variable.quality, VariableInfo::Quality::Good);
    QCOMPARE(variable.simulation.function, SimulationFunction::None);
    QCOMPARE(variable.simulation.intervalMs, 1000);
    QVERIFY(!variable.historizing);
    QCOMPARE(result.project.server.history.maxValues, HistoryLimits().maxValues);
    QCOMPARE(result.project.server.history.maxAgeMinutes, 0);
    const DiscoveryRegistration discovery = result.project.server.discovery;
    QVERIFY(!discovery.enabled);
    QCOMPARE(discovery.url, DiscoveryRegistration().url);
    QCOMPARE(discovery.intervalSeconds, DiscoveryRegistration().intervalSeconds);
    QVERIFY(!discovery.verifyCertificate);
}

void TestProjectFile::preservesExplicitHostname_data()
{
    QTest::addColumn<QString>("hostname");
    QTest::newRow("localhost") << QStringLiteral("localhost");
    QTest::newRow("remote") << QStringLiteral("plant-server");
    QTest::newRow("address") << QStringLiteral("192.168.1.100");
    QTest::newRow("empty") << QString();
}

void TestProjectFile::preservesExplicitHostname()
{
    QFETCH(QString, hostname);
    ProjectConfig project = DefaultProject::create();
    project.endpoints.first().hostname = hostname;
    const auto result = ProjectFile::fromJson(ProjectFile::toJson(project));
    QVERIFY2(result.ok(), qPrintable(result.error));
    QCOMPARE(result.project.endpoints.first().hostname, hostname);
}

void TestProjectFile::marksMachineHostname()
{
    const QByteArray json = ProjectFile::toJson(DefaultProject::create());
    const QJsonObject endpoint =
        QJsonDocument::fromJson(json).object().value(QStringLiteral("endpoints")).toArray().first().toObject();
    QVERIFY(endpoint.value(QStringLiteral("machineHostname")).toBool());
    QCOMPARE(endpoint.value(QStringLiteral("hostname")).toString(), QSysInfo::machineHostName());

    ProjectConfig project = DefaultProject::create();
    project.endpoints.first().hostname = QStringLiteral("plant-server");
    const QJsonObject explicitHost = QJsonDocument::fromJson(ProjectFile::toJson(project))
                                         .object().value(QStringLiteral("endpoints")).toArray().first().toObject();
    QVERIFY(!explicitHost.value(QStringLiteral("machineHostname")).toBool());
}

void TestProjectFile::machineHostnameFollowsLoadingComputer()
{
    QJsonObject root = QJsonDocument::fromJson(ProjectFile::toJson(DefaultProject::create())).object();
    QJsonArray endpoints = root.value(QStringLiteral("endpoints")).toArray();
    QJsonObject endpoint = endpoints.first().toObject();
    endpoint.insert(QStringLiteral("hostname"), QStringLiteral("other-computer"));
    endpoints.replace(0, endpoint);
    root.insert(QStringLiteral("endpoints"), endpoints);

    const ProjectFile::LoadResult result = ProjectFile::fromJson(QJsonDocument(root).toJson());
    QVERIFY2(result.ok(), qPrintable(result.error));
    QCOMPARE(result.project.endpoints.first().hostname, QSysInfo::machineHostName());
}

void TestProjectFile::keepsTrustedClientsOnlyKey()
{
    ProjectConfig original = DefaultProject::create();
    original.endpoints.first().trustedClientsOnly = true;
    const QByteArray json = ProjectFile::toJson(original);
    QVERIFY(json.contains("requireClientCertificate"));
    QVERIFY(ProjectFile::fromJson(json).project.endpoints.first().trustedClientsOnly);
}

void TestProjectFile::keepsUserCertificateFlag()
{
    ProjectConfig original = DefaultProject::create();
    original.endpoints.last().userCertificate = true;
    const ProjectConfig loaded = ProjectFile::fromJson(ProjectFile::toJson(original)).project;
    QVERIFY(!loaded.endpoints.first().userCertificate);
    QVERIFY(loaded.endpoints.last().userCertificate);

    QJsonObject json = QJsonDocument::fromJson(ProjectFile::toJson(original)).object();
    QJsonArray endpoints = json.value(QStringLiteral("endpoints")).toArray();
    QJsonObject first = endpoints.first().toObject();
    first.remove(QStringLiteral("userCertificate"));
    endpoints.replace(0, first);
    json.insert(QStringLiteral("endpoints"), endpoints);
    QVERIFY2(!ProjectFile::fromJson(QJsonDocument(json).toJson()).project.endpoints.first().userCertificate,
             "projects saved before the flag existed keep certificates off");
}

void TestProjectFile::rejectsMalformedJson()
{
    const ProjectFile::LoadResult result = ProjectFile::fromJson("{ \"formatVersion\": ");
    QVERIFY(!result.ok());
    QVERIFY(result.error.startsWith(QStringLiteral("Malformed project file")));
}

void TestProjectFile::rejectsNonObjectRoot()
{
    QVERIFY(!ProjectFile::fromJson("[1, 2]").ok());
}

void TestProjectFile::rejectsMissingVersion()
{
    const ProjectFile::LoadResult result = ProjectFile::fromJson("{}");
    QCOMPARE(result.error, QStringLiteral("Not an OpenUaSim project file"));
}

void TestProjectFile::rejectsNewerVersion()
{
    const ProjectFile::LoadResult result = ProjectFile::fromJson("{ \"formatVersion\": 99 }");
    QVERIFY(!result.ok());
    QVERIFY(result.error.contains(QStringLiteral("newer version")));
}

void TestProjectFile::rejectsInvalidNode_data()
{
    QTest::addColumn<QByteArray>("json");
    QTest::addColumn<QString>("expectedError");

    QJsonObject unknownKind = variableNode(QStringLiteral("kind"), QStringLiteral("view"));
    QJsonObject noName = variableNode(QStringLiteral("browseName"), QString());

    QTest::newRow("kind") << projectWithNode(unknownKind) << "nodes[0]: unknown node kind 'view'";
    QTest::newRow("name") << projectWithNode(noName) << "nodes[0]: browse name is empty";
    QTest::newRow("data type") << projectWithNode(variableNode(QStringLiteral("dataType"), QStringLiteral("Guid")))
                               << "nodes[0]: unsupported data type 'Guid'";
    QTest::newRow("access") << projectWithNode(variableNode(QStringLiteral("access"), QStringLiteral("Execute")))
                            << "nodes[0]: unknown access level 'Execute'";
    const QJsonObject sawtooth { { QStringLiteral("function"), QStringLiteral("sawtooth") } };
    QTest::newRow("simulation") << projectWithNode(variableNode(QStringLiteral("simulation"), sawtooth))
                                << "nodes[0]: unknown simulation function 'sawtooth'";
    QTest::newRow("quality") << projectWithNode(variableNode(QStringLiteral("quality"), QStringLiteral("fair")))
                             << "nodes[0]: unknown quality 'fair'";
    QTest::newRow("array length") << projectWithNode(variableNode(QStringLiteral("arrayLength"), 1001))
                                  << "nodes[0]: array length 1001 is outside 0..1000";
    const QJsonObject method { { QStringLiteral("kind"), QStringLiteral("method") },
                               { QStringLiteral("browseName"), QStringLiteral("Stop") },
                               { QStringLiteral("action"), QStringLiteral("explode") } };
    QTest::newRow("method action") << projectWithNode(method) << "nodes[0]: unknown method action 'explode'";
    const QJsonObject crossed { { QStringLiteral("low"), 90.0 }, { QStringLiteral("high"), 10.0 } };
    QTest::newRow("alarm") << projectWithNode(variableNode(QStringLiteral("alarm"), crossed))
                           << "nodes[0]: The limits must rise from LowLow through Low and High to HighHigh";

    const QJsonObject folder {
        { QStringLiteral("kind"), QStringLiteral("folder") },
        { QStringLiteral("browseName"), QStringLiteral("F") },
        { QStringLiteral("children"), QJsonArray { variableNode(QStringLiteral("dataType"), QStringLiteral("X")) } },
    };
    QTest::newRow("nested") << projectWithNode(folder) << "nodes[0].children[0]: unsupported data type 'X'";
}

void TestProjectFile::rejectsInvalidNode()
{
    QFETCH(QByteArray, json);
    QFETCH(QString, expectedError);

    const ProjectFile::LoadResult result = ProjectFile::fromJson(json);
    QCOMPARE(result.error, QStringLiteral("Invalid project file: ") + expectedError);
    QVERIFY(result.project.nodes.isEmpty());
}

void TestProjectFile::rejectsDuplicateNodeIds()
{
    ProjectConfig project = DefaultProject::create();
    NodeDefinition &motor = project.nodes.first().children.first();
    motor.children[1].nodeId = motor.children[0].nodeId;

    const ProjectFile::LoadResult result = ProjectFile::fromJson(ProjectFile::toJson(project));
    QVERIFY(!result.ok());
    QVERIFY(result.error.contains(QStringLiteral("'ns=1;s=Motor_01.Speed' is used more than once")));
}

void TestProjectFile::savesAndLoadsFile()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString fileName = directory.filePath(QStringLiteral("line.ouasim"));

    QString error;
    QVERIFY(ProjectFile::save(DefaultProject::create(), fileName, &error));
    QVERIFY(error.isEmpty());

    const ProjectFile::LoadResult result = ProjectFile::load(fileName);
    QVERIFY2(result.ok(), qPrintable(result.error));
    QCOMPARE(result.project.nodes.first().browseName, QStringLiteral("FactoryLine_01"));
}

void TestProjectFile::preservesModelVariables()
{
    ModelVariable simulated;
    simulated.namespaceUri = QStringLiteral("urn:test:tank");
    simulated.variable.kind = NodeDefinition::Kind::Variable;
    simulated.variable.browseName = QStringLiteral("Level");
    simulated.variable.nodeId = QStringLiteral("i=1001");
    simulated.variable.access = QStringLiteral("Read");
    simulated.variable.quality = VariableInfo::Quality::Uncertain;
    simulated.variable.simulation.function = SimulationFunction::Sine;
    simulated.variable.simulation.amplitude = 5.0;
    simulated.variable.alarm.enabled = true;
    simulated.variable.alarm.highEnabled = true;
    simulated.variable.alarm.high = 90.0;
    ModelVariable started = simulated;
    started.namespaceUri = QStringLiteral("urn:test:boiler");
    started.variable.initialValue = 12.5;
    started.variable.alarm = AlarmSettings();
    ProjectConfig project;
    project.modelVariables = { simulated, started };

    const ProjectFile::LoadResult result = ProjectFile::fromJson(ProjectFile::toJson(project));
    QVERIFY2(result.ok(), qPrintable(result.error));
    QCOMPARE(result.project.modelVariables.size(), 2);
    const ModelVariable &first = result.project.modelVariables.at(0);
    QCOMPARE(first.namespaceUri, simulated.namespaceUri);
    QCOMPARE(first.variable.nodeId, QStringLiteral("i=1001"));
    QCOMPARE(first.variable.access, QStringLiteral("Read"));
    QCOMPARE(first.variable.quality, VariableInfo::Quality::Uncertain);
    QCOMPARE(first.variable.simulation.function, SimulationFunction::Sine);
    QCOMPARE(first.variable.simulation.amplitude, 5.0);
    QVERIFY(first.variable.alarm == simulated.variable.alarm);
    QVERIFY(!first.variable.initialValue.isValid());
    const ModelVariable &second = result.project.modelVariables.at(1);
    QCOMPARE(second.namespaceUri, QStringLiteral("urn:test:boiler"));
    QCOMPARE(second.variable.nodeId, QStringLiteral("i=1001"));
    QCOMPARE(second.variable.initialValue.toDouble(), 12.5);
}

void TestProjectFile::rejectsInvalidModelVariable_data()
{
    QTest::addColumn<QByteArray>("json");
    QTest::addColumn<QString>("expectedError");

    const QString uri = QStringLiteral("urn:test:tank");
    QTest::newRow("namespace") << projectWithModelVariables({ modelVariable(QString(), QStringLiteral("i=1")) })
                               << "modelVariables[0]: namespace URI is empty";
    QTest::newRow("node id") << projectWithModelVariables({ modelVariable(uri, QString()) })
                             << "modelVariables[0]: node id is empty";
    QJsonObject folder = modelVariable(uri, QStringLiteral("i=1"));
    folder.insert(QStringLiteral("kind"), QStringLiteral("folder"));
    QTest::newRow("kind") << projectWithModelVariables({ folder })
                          << "modelVariables[0]: model settings belong to a variable";
    QTest::newRow("twice") << projectWithModelVariables({ modelVariable(uri, QStringLiteral("i=1")),
                                                          modelVariable(uri, QStringLiteral("i=1")) })
                           << "modelVariables[1]: the variable has settings more than once";
    QJsonObject access = modelVariable(uri, QStringLiteral("i=1"));
    access.insert(QStringLiteral("access"), QStringLiteral("Execute"));
    QTest::newRow("access") << projectWithModelVariables({ access })
                            << "modelVariables[0]: unknown access level 'Execute'";
}

void TestProjectFile::rejectsInvalidModelVariable()
{
    QFETCH(QByteArray, json);
    QFETCH(QString, expectedError);

    const ProjectFile::LoadResult result = ProjectFile::fromJson(json);
    QCOMPARE(result.error, QStringLiteral("Invalid project file: ") + expectedError);
}

void TestProjectFile::preservesModelMethods()
{
    ModelMethod failing { QStringLiteral("urn:test:doser"), QStringLiteral("i=5001"), {} };
    failing.response.statusCode = 0x80AF0000;
    ModelMethod answering { QStringLiteral("urn:test:doser"), QStringLiteral("i=5010"), {} };
    answering.response.outputs = { QStringLiteral("1.5"), QString(), QStringLiteral("1; 2") };
    ProjectConfig project;
    project.modelMethods = { failing, answering };

    const QByteArray json = ProjectFile::toJson(project);
    const ProjectFile::LoadResult result = ProjectFile::fromJson(json);

    QVERIFY2(result.ok(), qPrintable(result.error));
    QVERIFY(json.contains("\"BadInvalidState\""));
    QCOMPARE(result.project.modelMethods.size(), 2);
    QCOMPARE(result.project.modelMethods.at(0).namespaceUri, failing.namespaceUri);
    QCOMPARE(result.project.modelMethods.at(0).identifier, failing.identifier);
    QVERIFY(result.project.modelMethods.at(0).response == failing.response);
    QCOMPARE(result.project.modelMethods.at(1).identifier, answering.identifier);
    QCOMPARE(result.project.modelMethods.at(1).response.outputs, answering.response.outputs);
}

void TestProjectFile::modelMethodWithoutStatusAnswersGood()
{
    QJsonObject method = modelMethod(QStringLiteral("urn:test:doser"), QStringLiteral("i=5001"), QString());
    method.remove(QStringLiteral("status"));

    const ProjectFile::LoadResult result = ProjectFile::fromJson(projectWithModelMethods({ method }));

    QVERIFY2(result.ok(), qPrintable(result.error));
    QCOMPARE(result.project.modelMethods.first().response.statusCode, 0u);
}

void TestProjectFile::rejectsInvalidModelMethod_data()
{
    QTest::addColumn<QByteArray>("json");
    QTest::addColumn<QString>("expectedError");

    const QString uri = QStringLiteral("urn:test:doser");
    const QString good = QStringLiteral("Good");
    QTest::newRow("namespace") << projectWithModelMethods({ modelMethod(QString(), QStringLiteral("i=1"), good) })
                               << "modelMethods[0]: namespace URI is empty";
    QTest::newRow("node id") << projectWithModelMethods({ modelMethod(uri, QString(), good) })
                             << "modelMethods[0]: node id is empty";
    QTest::newRow("status") << projectWithModelMethods({ modelMethod(uri, QStringLiteral("i=1"),
                                                                     QStringLiteral("BadSomething")) })
                            << "modelMethods[0]: unknown status 'BadSomething'";
    QTest::newRow("twice") << projectWithModelMethods({ modelMethod(uri, QStringLiteral("i=1"), good),
                                                        modelMethod(uri, QStringLiteral("i=1"), good) })
                           << "modelMethods[1]: the method has settings more than once";
}

void TestProjectFile::rejectsInvalidModelMethod()
{
    QFETCH(QByteArray, json);
    QFETCH(QString, expectedError);

    const ProjectFile::LoadResult result = ProjectFile::fromJson(json);
    QCOMPARE(result.error, QStringLiteral("Invalid project file: ") + expectedError);
}

void TestProjectFile::storesModelPathsRelativeToFile()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString fileName = directory.filePath(QStringLiteral("line.ouasim"));
    ProjectConfig project;
    project.nodeSetFiles = { directory.filePath(QStringLiteral("line.models/tank.NodeSet2.xml")) };

    QString error;
    QVERIFY2(ProjectFile::save(project, fileName, &error), qPrintable(error));
    QFile file(fileName);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    const QJsonArray stored = root.value(QStringLiteral("nodeSetFiles")).toArray();
    QCOMPARE(stored, QJsonArray { QStringLiteral("line.models/tank.NodeSet2.xml") });

    const ProjectFile::LoadResult result = ProjectFile::load(fileName);
    QVERIFY2(result.ok(), qPrintable(result.error));
    QCOMPARE(result.project.nodeSetFiles, project.nodeSetFiles);
}

void TestProjectFile::loadReportsMissingFile()
{
    QTemporaryDir directory;
    const ProjectFile::LoadResult result = ProjectFile::load(directory.filePath(QStringLiteral("none.ouasim")));
    QVERIFY(!result.ok());
    QVERIFY(result.error.startsWith(QStringLiteral("Cannot open")));
}

void TestProjectFile::saveReportsUnwritablePath()
{
    QTemporaryDir directory;
    QString error;
    const QString fileName = directory.filePath(QStringLiteral("missing/dir/line.ouasim"));
    QVERIFY(!ProjectFile::save(ProjectConfig(), fileName, &error));
    QVERIFY(!error.isEmpty());
}

QTEST_GUILESS_MAIN(TestProjectFile)

#include "test_projectfile.moc"
