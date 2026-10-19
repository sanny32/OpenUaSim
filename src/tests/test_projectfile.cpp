// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
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

}

class TestProjectFile : public QObject
{
    Q_OBJECT

private slots:
    void defaultProjectRoundTrips();
    void preservesVariableFields();
    void preservesEveryDataTypeValue_data();
    void preservesEveryDataTypeValue();
    void preservesQuality();
    void preservesUsers();
    void movesFormat1EndpointLoginsToUsers();
    void emptyProjectRoundTrips();
    void missingFieldsUseDefaults();
    void keepsTrustedClientsOnlyKey();
    void rejectsMalformedJson();
    void rejectsNonObjectRoot();
    void rejectsMissingVersion();
    void rejectsNewerVersion();
    void rejectsInvalidNode_data();
    void rejectsInvalidNode();
    void rejectsDuplicateNodeIds();
    void savesAndLoadsFile();
    void loadReportsMissingFile();
    void saveReportsUnwritablePath();
};

void TestProjectFile::defaultProjectRoundTrips()
{
    const ProjectConfig original = DefaultProject::create();
    const ProjectFile::LoadResult result = ProjectFile::fromJson(ProjectFile::toJson(original));
    QVERIFY2(result.ok(), qPrintable(result.error));
    QCOMPARE(ProjectFile::toJson(result.project), ProjectFile::toJson(original));
}

void TestProjectFile::preservesVariableFields()
{
    const ProjectConfig original = DefaultProject::create();
    const ProjectConfig loaded = ProjectFile::fromJson(ProjectFile::toJson(original)).project;

    const NodeDefinition &speed = firstVariable(loaded);
    QCOMPARE(speed.kind, NodeDefinition::Kind::Variable);
    QCOMPARE(speed.browseName, QStringLiteral("Speed"));
    QCOMPARE(speed.nodeId, QStringLiteral("ns=1;s=Motor_01.Speed"));
    QCOMPARE(speed.icon, QStringLiteral("signal"));
    QCOMPARE(speed.dataType, QStringLiteral("Double"));
    QCOMPARE(speed.access, QStringLiteral("Read/Write"));
    QCOMPARE(speed.initialValue, QVariant(1000.0));
    QCOMPARE(speed.simulation.function, SimulationFunction::Sine);
    QCOMPARE(speed.simulation.intervalMs, 100);
    QCOMPARE(speed.simulation.periodMs, 10000);
    QCOMPARE(speed.simulation.amplitude, 500.0);
    QCOMPARE(speed.simulation.offset, 1000.0);
    QCOMPARE(loaded.endpoints.size(), 1);
    QCOMPARE(loaded.endpoints.first().url(), QStringLiteral("opc.tcp://localhost:48400"));
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
}

void TestProjectFile::keepsTrustedClientsOnlyKey()
{
    ProjectConfig original = DefaultProject::create();
    original.endpoints.first().trustedClientsOnly = true;
    const QByteArray json = ProjectFile::toJson(original);
    QVERIFY(json.contains("requireClientCertificate"));
    QVERIFY(ProjectFile::fromJson(json).project.endpoints.first().trustedClientsOnly);
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

    QJsonObject unknownKind = variableNode(QStringLiteral("kind"), QStringLiteral("method"));
    QJsonObject noName = variableNode(QStringLiteral("browseName"), QString());

    QTest::newRow("kind") << projectWithNode(unknownKind) << "nodes[0]: unknown node kind 'method'";
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
