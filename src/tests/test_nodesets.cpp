// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QXmlStreamReader>

#include <QUaServer>

#include <algorithm>

#include "project/defaultproject.h"
#include "server/importednodes.h"
#include "server/servercontroller.h"
#include "testport.h"
#include "uatestclient.h"

namespace {

QString testData(const QString &name)
{
    return QStringLiteral(OUASIM_TEST_DATA_DIR "/") + name;
}

const QString kApplicationUri = QStringLiteral("urn:openuasim:server");

QByteArray exportProject(ServerController &controller, const QString &fileName)
{
    QString error;
    if (!controller.exportNodeSet(fileName, &error))
        qWarning() << error;
    QFile file(fileName);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

bool logContains(const QList<LogEntry> &log, const QString &text)
{
    for (const LogEntry &entry : log) {
        if (entry.message.contains(text))
            return true;
    }
    return false;
}

}

class TestNodeSets : public QObject
{
    Q_OBJECT

private slots:
    void writesWellFormedDocument();
    void writesVariableAttributes();
    void exportLoadsIntoAnotherServer();
    void importAddsModelToProject();
    void importRejectsDuplicate();
    void importReportsBrokenFile();
    void setProjectLoadsItsModels();
    void removeUnloadsModel();
    void removeRestartsRunningServer();
    void removeRejectsUnknownModel();
    void snapshotListsImportedNodes();
    void namesBuiltInDataTypes();
    void exportWritesFile();
    void exportsImportedNamespace();
    void exportRefusesStandardNamespace();
    void exportReportsUnknownNamespace();
    void exportReportsUnwritablePath();
    void examplesLoadCleanly_data();
    void examplesLoadCleanly();

private:
    QTemporaryDir _directory;
};

void TestNodeSets::writesWellFormedDocument()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    const QByteArray document = exportProject(controller, _directory.filePath(QStringLiteral("project.xml")));
    QXmlStreamReader xml(document);
    int objects = 0;
    int variables = 0;
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement())
            continue;
        if (xml.name() == QLatin1String("UAObject"))
            ++objects;
        else if (xml.name() == QLatin1String("UAVariable"))
            ++variables;
    }
    QVERIFY2(!xml.hasError(), qPrintable(xml.errorString()));
    QCOMPARE(objects, 5);
    QCOMPARE(variables, 12);
}

void TestNodeSets::writesVariableAttributes()
{
    NodeDefinition fixed;
    fixed.kind = NodeDefinition::Kind::Variable;
    fixed.browseName = QStringLiteral("Fixed");
    fixed.nodeId = QStringLiteral("ns=1;s=Fixed");
    fixed.dataType = QStringLiteral("Int16");
    fixed.access = QStringLiteral("Read");
    fixed.initialValue = 7;
    ProjectConfig project;
    project.nodes = { fixed };
    ServerController controller;
    controller.setProject(project);

    const QString fileName = _directory.filePath(QStringLiteral("fixed.xml"));
    const QString document = QString::fromUtf8(exportProject(controller, fileName));
    QVERIFY(document.contains(QStringLiteral("NodeId=\"ns=1;s=Fixed\"")));
    QVERIFY(document.contains(QStringLiteral("BrowseName=\"1:Fixed\"")));
    QVERIFY(document.contains(QStringLiteral("DataType=\"Int16\"")));
    QVERIFY(document.contains(QStringLiteral("<uax:Int16>7</uax:Int16>")));

    QUaServer server;
    QVERIFY(server.loadNodeSet(fileName).isOk());
    auto *loaded = server.nodeById<QUaBaseDataVariable>(QUaNodeId(QStringLiteral("ns=1;s=Fixed")));
    QVERIFY(loaded);
    QVERIFY(loaded->readAccess());
    QVERIFY(!loaded->writeAccess());
}

void TestNodeSets::exportLoadsIntoAnotherServer()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    const QString fileName = _directory.filePath(QStringLiteral("roundtrip.xml"));
    exportProject(controller, fileName);

    QUaServer server;
    const QUaNodeSetResult result = server.loadNodeSet(fileName);
    QVERIFY2(result.isOk(), qPrintable(result.errorString));
    QVERIFY2(result.warnings.isEmpty(), qPrintable(result.warnings.join(QStringLiteral("; "))));
    QCOMPARE(result.addedNodes.size(), 17);

    auto *speed = server.nodeById<QUaBaseDataVariable>(QUaNodeId(QStringLiteral("ns=1;s=Motor_01.Speed")));
    QVERIFY(speed);
    QCOMPARE(speed->value().toDouble(), 1000.0);
    QVERIFY(speed->writeAccess());
}

void TestNodeSets::importAddsModelToProject()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    QSignalSpy edited(&controller, &ServerController::projectEdited);

    QVERIFY(controller.importNodeSet(testData(QStringLiteral("mixer.NodeSet2.xml"))));
    QCOMPARE(edited.count(), 1);
    QCOMPARE(controller.project().nodeSetFiles.size(), 1);
    QVERIFY(controller.server().uaServer()->namespaces().contains(QStringLiteral("urn:openuasim:test:mixer")));
    QVERIFY(logContains(controller.log(), QStringLiteral("loaded with 2 nodes")));
    QCOMPARE(controller.snapshot().namespaces.size(), controller.server().uaServer()->namespaces().size());
}

void TestNodeSets::importRejectsDuplicate()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    QVERIFY(controller.importNodeSet(testData(QStringLiteral("mixer.NodeSet2.xml"))));
    QString error;
    QVERIFY(!controller.importNodeSet(testData(QStringLiteral("mixer.NodeSet2.xml")), &error));
    QVERIFY(error.contains(QStringLiteral("already imported")));
}

void TestNodeSets::importReportsBrokenFile()
{
    const QString fileName = _directory.filePath(QStringLiteral("broken.xml"));
    QFile file(fileName);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("<UANodeSet><UAObject");
    file.close();

    ServerController controller;
    controller.setProject(DefaultProject::create());
    QString error;
    QVERIFY(!controller.importNodeSet(fileName, &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(controller.project().nodeSetFiles.isEmpty());
    QVERIFY(logContains(controller.log(), QStringLiteral("not loaded")));
}

void TestNodeSets::setProjectLoadsItsModels()
{
    ProjectConfig project = DefaultProject::create();
    project.nodeSetFiles = { testData(QStringLiteral("mixer.NodeSet2.xml")) };
    ServerController controller;
    controller.setProject(project);
    QVERIFY(controller.server().uaServer()->namespaces().contains(QStringLiteral("urn:openuasim:test:mixer")));

    controller.setProject(DefaultProject::create());
    QVERIFY(!controller.server().uaServer()->namespaces().contains(QStringLiteral("urn:openuasim:test:mixer")));
    QVERIFY(controller.addressSpace().hasVariable(QStringLiteral("ns=1;s=Motor_01.Speed")));
}

void TestNodeSets::removeUnloadsModel()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    const QString mixer = testData(QStringLiteral("mixer.NodeSet2.xml"));
    QVERIFY(controller.importNodeSet(mixer));
    const QString path = controller.project().nodeSetFiles.first();
    QCOMPARE(controller.modelNamespaces().value(QStringLiteral("urn:openuasim:test:mixer")), path);
    QSignalSpy edited(&controller, &ServerController::projectEdited);

    QVERIFY(controller.removeNodeSet(path));
    QCOMPARE(edited.count(), 1);
    QVERIFY(controller.project().nodeSetFiles.isEmpty());
    QVERIFY(controller.modelNamespaces().isEmpty());
    QVERIFY(!controller.server().uaServer()->namespaces().contains(QStringLiteral("urn:openuasim:test:mixer")));
    QVERIFY(controller.addressSpace().hasVariable(QStringLiteral("ns=1;s=Motor_01.Speed")));
    QVERIFY(controller.importNodeSet(mixer));
}

void TestNodeSets::removeRestartsRunningServer()
{
    ProjectConfig project = DefaultProject::create();
    project.endpoints.first().port = TestPort::freePort();
    project.nodeSetFiles = { testData(QStringLiteral("mixer.NodeSet2.xml")) };
    ServerController controller;
    controller.setProject(project);
    QVERIFY(controller.start());

    QVERIFY(controller.removeNodeSet(project.nodeSetFiles.first()));
    QVERIFY(controller.isRunning());
    UaTestClient client;
    QCOMPARE(client.connect(controller.server().endpointUrl()), UA_STATUSCODE_GOOD);
    QVariant value;
    QCOMPARE(client.readValue(QStringLiteral("ns=1;s=Motor_01.Speed"), value), UA_STATUSCODE_GOOD);
}

void TestNodeSets::removeRejectsUnknownModel()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    QString error;
    QVERIFY(!controller.removeNodeSet(QStringLiteral("missing.xml"), &error));
    QVERIFY(error.contains(QStringLiteral("not imported")));
}

void TestNodeSets::snapshotListsImportedNodes()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    QVERIFY(controller.importNodeSet(testData(QStringLiteral("mixer.NodeSet2.xml"))));
    const ServerSnapshot snapshot = controller.snapshot();

    const QList<TreeNode> &objects = snapshot.addressSpace.first().children;
    const auto mixer = std::find_if(objects.cbegin(), objects.cend(),
                                    [](const TreeNode &node) { return node.text == QLatin1String("Mixer"); });
    QVERIFY(mixer != objects.cend());
    QCOMPARE(mixer->kind, TreeNode::Kind::Object);
    QCOMPARE(mixer->children.size(), 1);
    QCOMPARE(mixer->children.first().kind, TreeNode::Kind::Variable);

    const QList<VariableInfo> variables = snapshot.variables.value(mixer->id);
    QCOMPARE(variables.size(), 1);
    QCOMPARE(variables.first().displayName, QStringLiteral("Speed"));
    QCOMPARE(variables.first().dataType, QStringLiteral("Double"));
    QCOMPARE(variables.first().value.toDouble(), 42.5);
    QCOMPARE(variables.first().access, QStringLiteral("Read/Write"));
    QVERIFY(variables.first().readOnly);

    const auto ns = std::find_if(snapshot.namespaces.cbegin(), snapshot.namespaces.cend(), [](const NamespaceInfo &info) {
        return info.uri == QLatin1String("urn:openuasim:test:mixer");
    });
    QVERIFY(ns != snapshot.namespaces.cend());
    QCOMPARE(ns->modelFile, controller.project().nodeSetFiles.first());
    QVERIFY(snapshot.namespaces.at(1).modelFile.isEmpty());
}

void TestNodeSets::namesBuiltInDataTypes()
{
    QCOMPARE(ImportedNodes::dataTypeName(QStringLiteral("i=1")), QStringLiteral("Boolean"));
    QCOMPARE(ImportedNodes::dataTypeName(QStringLiteral("ns=0;i=13")), QStringLiteral("DateTime"));
    QCOMPARE(ImportedNodes::dataTypeName(QStringLiteral("ns=2;i=3002")), QStringLiteral("ns=2;i=3002"));
}

void TestNodeSets::exportWritesFile()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    const QString fileName = _directory.filePath(QStringLiteral("export.xml"));
    QVERIFY(controller.exportNodeSet(fileName));
    QFile file(fileName);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QVERIFY(file.readAll().contains("Motor_01.Speed"));
}

void TestNodeSets::exportsImportedNamespace()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    QVERIFY(controller.importNodeSet(testData(QStringLiteral("mixer.NodeSet2.xml"))));
    const QString fileName = _directory.filePath(QStringLiteral("mixer.xml"));
    QString error;
    QVERIFY2(controller.exportNamespace(fileName, QStringLiteral("urn:openuasim:test:mixer"), &error),
             qPrintable(error));

    QUaServer server;
    const QUaNodeSetResult result = server.loadNodeSet(fileName);
    QVERIFY2(result.isOk(), qPrintable(result.errorString));
    QCOMPARE(result.addedNodes.size(), 2);
    QFile file(fileName);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QVERIFY(!file.readAll().contains("Motor_01.Speed"));
}

void TestNodeSets::exportRefusesStandardNamespace()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    const QString fileName = _directory.filePath(QStringLiteral("ns0.xml"));
    QString error;
    QVERIFY(!controller.exportNamespace(fileName, QStringLiteral("http://opcfoundation.org/UA/"), &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!QFile::exists(fileName));
}

void TestNodeSets::exportReportsUnknownNamespace()
{
    ServerController controller;
    controller.setProject(DefaultProject::create());
    QString error;
    QVERIFY(!controller.exportNamespace(_directory.filePath(QStringLiteral("unknown.xml")),
                                        QStringLiteral("urn:no:such:namespace"), &error));
    QVERIFY(!error.isEmpty());
}

void TestNodeSets::exportReportsUnwritablePath()
{
    ServerController controller;
    QString error;
    QVERIFY(!controller.exportNodeSet(_directory.filePath(QStringLiteral("no/such/dir/x.xml")), &error));
    QVERIFY(!error.isEmpty());
}

void TestNodeSets::examplesLoadCleanly_data()
{
    QTest::addColumn<QString>("fileName");
    const QDir examples(QStringLiteral(OUASIM_EXAMPLE_NODESETS_DIR));
    if (!examples.exists())
        QSKIP("The example NodeSets are not part of this source tree");
    const QFileInfoList files = examples.entryInfoList({ QStringLiteral("*.NodeSet2.xml") }, QDir::Files);
    QVERIFY(!files.isEmpty());
    for (const QFileInfo &file : files)
        QTest::newRow(qPrintable(file.fileName())) << file.absoluteFilePath();
}

void TestNodeSets::examplesLoadCleanly()
{
    QFETCH(QString, fileName);
    ServerController controller;
    controller.setProject(DefaultProject::create());
    const qsizetype logBefore = controller.log().size();

    QString error;
    QVERIFY2(controller.importNodeSet(fileName, &error), qPrintable(error));
    const QList<LogEntry> &log = controller.log();
    for (qsizetype i = logBefore; i < log.size(); ++i)
        QVERIFY2(log.at(i).level == LogEntry::Level::Info, qPrintable(log.at(i).message));
}

QTEST_GUILESS_MAIN(TestNodeSets)

#include "test_nodesets.moc"
