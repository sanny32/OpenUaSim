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

#include "server/importednodes.h"
#include "server/servercontroller.h"
#include "server/typecatalog.h"
#include "testport.h"
#include "testproject.h"
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

const QString kTankUri = QStringLiteral("urn:openuasim:test:tank");

ProjectConfig tankProject()
{
    ProjectConfig project = TestProject::create();
    project.endpoints.first().port = TestPort::freePort();
    project.nodeSetFiles = { testData(QStringLiteral("tank.NodeSet2.xml")) };
    return project;
}

///
/// \brief Provides the project container referenced by the imported hierarchy fixture.
///
ProjectConfig branchProject()
{
    NodeDefinition host;
    host.kind = NodeDefinition::Kind::Object;
    host.browseName = QStringLiteral("Host");
    host.nodeId = QStringLiteral("ns=1;s=Host");
    ProjectConfig project = TestProject::create();
    project.endpoints.first().port = TestPort::freePort();
    project.nodes = {host};
    return project;
}

///
/// \brief Loads the display fixture into a running server, whose namespace 1 then has the project URI it references.
///
QUaNodeSetResult loadBranches(ServerController &controller)
{
    QUaServer *server = controller.server().uaServer();
    return server->loadNodeSet(testData(QStringLiteral("imported-branches.NodeSet2.xml")));
}

VariableInfo modelVariable(const ServerController &controller, const QString &name)
{
    const ServerSnapshot snapshot = controller.snapshot();
    for (const QList<VariableInfo> &variables : snapshot.variables) {
        for (const VariableInfo &variable : variables) {
            if (variable.fromModel && variable.displayName == name)
                return variable;
        }
    }
    return {};
}

const TreeNode *findTreeNode(const QList<TreeNode> &nodes, const QString &id)
{
    for (const TreeNode &node : nodes) {
        if (node.id == id)
            return &node;
        if (const TreeNode *found = findTreeNode(node.children, id))
            return found;
    }
    return nullptr;
}

///
/// \brief Counts occurrences across the displayed hierarchy to catch duplicate branches and reference cycles.
///
int treeOccurrences(const QList<TreeNode> &nodes, const QString &id)
{
    int result = 0;
    for (const TreeNode &node : nodes) {
        if (node.id == id)
            ++result;
        result += treeOccurrences(node.children, id);
    }
    return result;
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
    void totalNodesTracksImportedModels();
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
    void publicModelsImport_data();
    void publicModelsImport();
    void snapshotKeepsImportedStandardPaths();
    void snapshotIncludesImportedPropertiesMethodsAndRootVariables();
    void snapshotSkipsEventNotifierLinks();
    void snapshotMergesImportedProjectChildren();
    void modelOwnsReferencedNamespaces();
    void modelOwnsOnlyNewNamespaces();
    void modelVariableOfUnsupportedTypeIsReadOnly();
    void modelEnumerationIsEditedAsInt32();
    void modelTypesAreListed();
    void updateModelVariableKeepsSettings();
    void updateModelVariableReplacesSettings();
    void updateModelVariableRejectsInvalidValue();
    void modelVariableRaisesAlarm();
    void modelVariableIsSimulated();
    void modelSettingsApplyWhenProjectLoads();
    void modelSettingsOfUnknownNamespaceAreLogged();
    void removeNodeSetDropsModelSettings();

private:
    QTemporaryDir _directory;
};

void TestNodeSets::writesWellFormedDocument()
{
    ServerController controller;
    controller.setProject(TestProject::create());
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
    fixed.icon = QStringLiteral("thermometer");
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
    project.nodes.first().icon.clear();
    controller.setProject(project);
    QCOMPARE(controller.snapshot().addressSpace.first().children.first().icon, QStringLiteral("variable-type"));
    QCOMPARE(QString::fromUtf8(exportProject(controller, fileName)), document);
}

void TestNodeSets::exportLoadsIntoAnotherServer()
{
    ServerController controller;
    controller.setProject(TestProject::create());
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
    controller.setProject(TestProject::create());
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
    controller.setProject(TestProject::create());
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
    controller.setProject(TestProject::create());
    QString error;
    QVERIFY(!controller.importNodeSet(fileName, &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(controller.project().nodeSetFiles.isEmpty());
    QVERIFY(logContains(controller.log(), QStringLiteral("not loaded")));
}

void TestNodeSets::setProjectLoadsItsModels()
{
    ProjectConfig project = TestProject::create();
    project.nodeSetFiles = { testData(QStringLiteral("mixer.NodeSet2.xml")) };
    ServerController controller;
    controller.setProject(project);
    QVERIFY(controller.server().uaServer()->namespaces().contains(QStringLiteral("urn:openuasim:test:mixer")));

    controller.setProject(TestProject::create());
    QVERIFY(!controller.server().uaServer()->namespaces().contains(QStringLiteral("urn:openuasim:test:mixer")));
    QVERIFY(controller.addressSpace().hasVariable(QStringLiteral("ns=1;s=Motor_01.Speed")));
}

void TestNodeSets::removeUnloadsModel()
{
    ServerController controller;
    controller.setProject(TestProject::create());
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
    ProjectConfig project = TestProject::create();
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
    controller.setProject(TestProject::create());
    QString error;
    QVERIFY(!controller.removeNodeSet(QStringLiteral("missing.xml"), &error));
    QVERIFY(error.contains(QStringLiteral("not imported")));
}

void TestNodeSets::snapshotListsImportedNodes()
{
    ServerController controller;
    controller.setProject(TestProject::create());
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
    QVERIFY(variables.first().icon.isEmpty());
    QCOMPARE(variables.first().typeIcon, QStringLiteral("variable-type"));
    QCOMPARE(variables.first().dataType, QStringLiteral("Double"));
    QCOMPARE(variables.first().value.toDouble(), 42.5);
    QCOMPARE(variables.first().access, QStringLiteral("Read/Write"));
    QVERIFY(variables.first().fromModel);
    QVERIFY(!variables.first().readOnly);

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
    controller.setProject(TestProject::create());
    const QString fileName = _directory.filePath(QStringLiteral("export.xml"));
    QVERIFY(controller.exportNodeSet(fileName));
    QFile file(fileName);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QVERIFY(file.readAll().contains("Motor_01.Speed"));
}

void TestNodeSets::exportsImportedNamespace()
{
    ServerController controller;
    controller.setProject(TestProject::create());
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
    controller.setProject(TestProject::create());
    const QString fileName = _directory.filePath(QStringLiteral("ns0.xml"));
    QString error;
    QVERIFY(!controller.exportNamespace(fileName, QStringLiteral("http://opcfoundation.org/UA/"), &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!QFile::exists(fileName));
}

void TestNodeSets::exportReportsUnknownNamespace()
{
    ServerController controller;
    controller.setProject(TestProject::create());
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
    controller.setProject(TestProject::create());
    const qsizetype logBefore = controller.log().size();

    QString error;
    QVERIFY2(controller.importNodeSet(fileName, &error), qPrintable(error));
    const QList<LogEntry> &log = controller.log();
    for (qsizetype i = logBefore; i < log.size(); ++i)
        QVERIFY2(log.at(i).level == LogEntry::Level::Info, qPrintable(log.at(i).message));
}

///
/// \brief Defines public companion models and their dependency load order.
///
void TestNodeSets::publicModelsImport_data()
{
    QTest::addColumn<QStringList>("models");
    QTest::addColumn<QString>("namespaceUri");
    QTest::addColumn<quint32>("rootIdentifier");
    QTest::addColumn<bool>("hasInstanceData");
    QTest::addColumn<int>("expectedWarnings");
    const QString di = QStringLiteral("Opc.Ua.Di.NodeSet2.xml");
    const QString ia = QStringLiteral("Opc.Ua.IA.NodeSet2.xml");
    const QString machinery = QStringLiteral("Opc.Ua.Machinery.NodeSet2.xml");
    const QString pumps = QStringLiteral("Opc.Ua.Pumps.NodeSet2.xml");
    QTest::newRow("DI") << QStringList { di }
                        << QStringLiteral("http://opcfoundation.org/UA/DI/") << quint32(0) << false << 0;
    QTest::newRow("ADI") << QStringList { di, "Opc.Ua.Adi.NodeSet2.xml" }
                         << QStringLiteral("http://opcfoundation.org/UA/ADI/") << quint32(0) << false << 0;
    QTest::newRow("AutoID") << QStringList { di, "Opc.Ua.AutoID.NodeSet2.xml" }
                            << QStringLiteral("http://opcfoundation.org/UA/AutoID/") << quint32(0) << false << 0;
    QTest::newRow("Machinery") << QStringList { di, ia, machinery }
                               << QStringLiteral("http://opcfoundation.org/UA/Machinery/") << quint32(0) << false << 0;
    QTest::newRow("DemoModel") << QStringList { "DemoModel.NodeSet2.xml" }
                               << QStringLiteral("urn:opcfoundation.org:2024-01:DemoModel") << quint32(351) << true << 2;
    QTest::newRow("IA examples") << QStringList { di, ia, "Opc.Ua.IA.NodeSet2.examples.xml" }
                                 << QStringLiteral("http://opcfoundation.org/UA/IA/Examples/") << quint32(5002) << true << 0;
    QTest::newRow("Pumps example") << QStringList { di, ia, machinery, pumps, "instanceexample.xml" }
                                   << QStringLiteral("http://yourorganisation.org/InstanceExample/") << quint32(5002) << true << 0;
    QTest::newRow("PackML") << QStringList { "Opc.Ua.PackML.NodeSet2.xml" }
                            << QStringLiteral("http://opcfoundation.org/UA/PackML/") << quint32(72) << false << 0;
}

///
/// \brief Checks public models through the application import and snapshot path.
///
void TestNodeSets::publicModelsImport()
{
    QFETCH(QStringList, models);
    QFETCH(QString, namespaceUri);
    QFETCH(quint32, rootIdentifier);
    QFETCH(bool, hasInstanceData);
    QFETCH(int, expectedWarnings);
    const QDir directory(testData(QStringLiteral("public-nodesets")));
    QStringList files;
    for (const QString &model : models) {
        const QString file = directory.filePath(model);
        QVERIFY2(QFile::exists(file), qPrintable(QStringLiteral("Missing bundled model: %1").arg(file)));
        files.append(file);
    }

    ServerController controller;
    controller.setProject(TestProject::create());
    for (const QString &file : files) {
        QString error;
        QVERIFY2(controller.importNodeSet(file, &error), qPrintable(file + QStringLiteral(": ") + error));
    }
    QCOMPARE(controller.project().nodeSetFiles, files);
    QVERIFY(controller.server().uaServer()->namespaces().contains(namespaceUri));
    const ServerSnapshot snapshot = controller.snapshot();
    QVERIFY(!snapshot.types.isEmpty());
    if (rootIdentifier != 0) {
        const quint16 ns = static_cast<quint16>(controller.server().uaServer()->namespaces().indexOf(namespaceUri));
        const QUaNodeId rootId(ns, rootIdentifier);
        const TreeNode *root = findTreeNode(snapshot.addressSpace, rootId.toXmlString());
        QVERIFY2(root, qPrintable(QStringLiteral("Imported root is absent from Objects: %1").arg(rootId.toXmlString())));
        if (hasInstanceData)
            QVERIFY2(!root->children.isEmpty(), qPrintable(root->text));
    }
    int warnings = 0;
    for (const LogEntry &entry : controller.log()) {
        QVERIFY2(entry.level != LogEntry::Level::Error, qPrintable(entry.message));
        if (entry.level == LogEntry::Level::Warning) {
            ++warnings;
            qInfo().noquote() << entry.message;
        }
    }
    qInfo() << "Imported" << files.size() << "models; snapshot types:" << snapshot.types.size()
            << "warnings:" << warnings;
    QCOMPARE(warnings, expectedWarnings);
}

///
/// \brief Preserves the real Server path to imported roles while pruning unrelated standard branches.
///
void TestNodeSets::snapshotKeepsImportedStandardPaths()
{
    ServerController controller;
    controller.setProject(TestProject::create());
    QVERIFY(controller.importNodeSet(testData(QStringLiteral("public-nodesets/DemoModel.NodeSet2.xml"))));
    const quint16 ns = static_cast<quint16>(controller.server().uaServer()->namespaces().indexOf(
        QStringLiteral("urn:opcfoundation.org:2024-01:DemoModel")));
    const QString seekerId = QUaNodeId(ns, quint32(95)).toXmlString();
    const QString identitiesId = QUaNodeId(ns, quint32(10)).toXmlString();
    const ServerSnapshot snapshot = controller.snapshot();
    const TreeNode *server = findTreeNode(snapshot.addressSpace.first().children, QStringLiteral("ns=0;i=2253"));
    QVERIFY(server);
    const TreeNode *capabilities = findTreeNode(server->children, QStringLiteral("ns=0;i=2268"));
    QVERIFY(capabilities);
    const TreeNode *roles = findTreeNode(capabilities->children, QStringLiteral("ns=0;i=15606"));
    QVERIFY(roles);
    const TreeNode *seeker = findTreeNode(roles->children, seekerId);
    QVERIFY(seeker);
    const TreeNode *identities = findTreeNode(seeker->children, identitiesId);
    QVERIFY(identities);
    QCOMPARE(identities->kind, TreeNode::Kind::Property);
    const QList<VariableInfo> properties = snapshot.variables.value(seekerId);
    const auto info = std::find_if(properties.cbegin(), properties.cend(),
                                   [&identitiesId](const VariableInfo &entry) { return entry.nodeId == identitiesId; });
    QVERIFY(info != properties.cend());
    QVERIFY(info->fromModel);
    QVERIFY(info->property);
    QVERIFY(info->readOnly);
    QVERIFY(!findTreeNode(server->children, QStringLiteral("ns=0;i=2255")));
    QCOMPARE(treeOccurrences(snapshot.addressSpace, seekerId), 1);
    QVERIFY(controller.removeNodeSet(controller.project().nodeSetFiles.first()));
    QVERIFY(!findTreeNode(controller.snapshot().addressSpace, QStringLiteral("ns=0;i=2253")));
}

///
/// \brief Covers native hierarchy links, custom references, methods, object properties and root variables.
///
void TestNodeSets::snapshotIncludesImportedPropertiesMethodsAndRootVariables()
{
    ServerController controller;
    controller.setProject(branchProject());
    QVERIFY(controller.start());
    const QUaNodeSetResult imported = loadBranches(controller);
    QVERIFY2(imported.isOk(), qPrintable(imported.errorString));
    QVERIFY2(imported.warnings.isEmpty(), qPrintable(imported.warnings.join(QLatin1Char('\n'))));
    const ServerSnapshot snapshot = controller.snapshot();
    const TreeNode *branch = findTreeNode(snapshot.addressSpace, QStringLiteral("ns=2;i=1000"));
    QVERIFY(branch);
    QCOMPARE(treeOccurrences(branch->children, QStringLiteral("ns=2;i=1001")), 1);
    QCOMPARE(treeOccurrences(branch->children, branch->id), 0);
    const TreeNode *property = findTreeNode(branch->children, QStringLiteral("ns=2;i=1100"));
    QVERIFY(property);
    QCOMPARE(property->kind, TreeNode::Kind::Property);
    const QList<VariableInfo> owned = snapshot.variables.value(branch->id);
    QCOMPARE(owned.size(), 1);
    QCOMPARE(owned.first().displayName, QStringLiteral("Label"));
    QCOMPARE(owned.first().value.toString(), QStringLiteral("fixture"));
    QVERIFY(owned.first().readOnly);
    const TreeNode *method = findTreeNode(branch->children, QStringLiteral("ns=2;i=7000"));
    QVERIFY(method);
    QCOMPARE(method->kind, TreeNode::Kind::Method);
    const TreeNode *root = findTreeNode(snapshot.addressSpace.first().children, QStringLiteral("ns=2;i=1200"));
    QVERIFY(root);
    QCOMPARE(root->kind, TreeNode::Kind::Variable);
    const QList<VariableInfo> rootVariables = snapshot.variables.value(QStringLiteral("i=85"));
    QCOMPARE(rootVariables.size(), 1);
    QCOMPARE(rootVariables.first().value.toDouble(), 42.0);
}

///
/// \brief Shows a node linked to Server by HasNotifier only at its Objects location.
///
void TestNodeSets::snapshotSkipsEventNotifierLinks()
{
    ServerController controller;
    controller.setProject(branchProject());
    QVERIFY(controller.start());
    const QUaNodeSetResult imported = loadBranches(controller);
    QVERIFY2(imported.isOk(), qPrintable(imported.errorString));
    const QString stationId = QStringLiteral("ns=2;i=1400");
    const ServerSnapshot snapshot = controller.snapshot();
    QCOMPARE(treeOccurrences(snapshot.addressSpace, stationId), 1);
    const QList<TreeNode> &objects = snapshot.addressSpace.first().children;
    QVERIFY(std::any_of(objects.cbegin(), objects.cend(),
                        [&stationId](const TreeNode &node) { return node.id == stationId; }));
}

///
/// \brief Attaches imported descendants to existing project containers without duplicating them.
///
void TestNodeSets::snapshotMergesImportedProjectChildren()
{
    const ProjectConfig project = branchProject();
    const NodeDefinition host = project.nodes.first();
    ServerController controller;
    controller.setProject(project);
    QVERIFY(controller.start());
    const QUaNodeSetResult imported = loadBranches(controller);
    QVERIFY2(imported.isOk(), qPrintable(imported.errorString));
    QVERIFY2(imported.warnings.isEmpty(), qPrintable(imported.warnings.join(QLatin1Char('\n'))));
    const ServerSnapshot snapshot = controller.snapshot();
    QCOMPARE(treeOccurrences(snapshot.addressSpace, host.nodeId), 1);
    const TreeNode *container = findTreeNode(snapshot.addressSpace, host.nodeId);
    QVERIFY(container);
    QCOMPARE(container->text, host.browseName);
    const TreeNode *extra = findTreeNode(container->children, QStringLiteral("ns=2;i=1300"));
    QVERIFY(extra);
    QCOMPARE(extra->text, QStringLiteral("Extra"));
    QCOMPARE(treeOccurrences(snapshot.addressSpace, extra->id), 1);
}

///
/// \brief Attributes namespaces a model only references to that model when no other model registered them first.
///
void TestNodeSets::modelOwnsReferencedNamespaces()
{
    const QStringList expected { QStringLiteral("urn:openuasim:test:owner"),
                                 QStringLiteral("urn:openuasim:test:referenced") };
    const QString example = _directory.filePath(QStringLiteral("referencing.xml"));
    QFile file(example);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QStringLiteral(
                   "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                   "<UANodeSet xmlns=\"http://opcfoundation.org/UA/2011/03/UANodeSet.xsd\">\n"
                   "  <NamespaceUris><Uri>%1</Uri><Uri>%2</Uri></NamespaceUris>\n"
                   "  <Models>\n"
                   "    <Model ModelUri=\"%1\"><RequiredModel ModelUri=\"http://opcfoundation.org/UA/\" /></Model>\n"
                   "  </Models>\n"
                   "  <UAObject NodeId=\"ns=1;i=1000\" BrowseName=\"1:Owner\">\n"
                   "    <DisplayName>Owner</DisplayName>\n"
                   "    <References>\n"
                   "      <Reference ReferenceType=\"i=35\" IsForward=\"false\">i=85</Reference>\n"
                   "      <Reference ReferenceType=\"i=40\">i=58</Reference>\n"
                   "    </References>\n"
                   "  </UAObject>\n"
                   "</UANodeSet>\n")
                   .arg(expected.at(0), expected.at(1))
                   .toUtf8());
    file.close();

    ServerController controller;
    controller.setProject(TestProject::create());
    QVERIFY(controller.importNodeSet(example));
    QCOMPARE(controller.modelNamespaceUris(example), expected);
    for (const NamespaceInfo &ns : controller.snapshot().namespaces)
        QCOMPARE(ns.modelFile, expected.contains(ns.uri) ? example : QString());
    QVERIFY(controller.modelNamespaceUris(testData(QStringLiteral("mixer.NodeSet2.xml"))).isEmpty());
}

///
/// \brief Leaves namespaces registered by earlier models with those models.
///
void TestNodeSets::modelOwnsOnlyNewNamespaces()
{
    ServerController controller;
    controller.setProject(TestProject::create());
    const QString di = testData(QStringLiteral("public-nodesets/Opc.Ua.Di.NodeSet2.xml"));
    const QString ia = testData(QStringLiteral("public-nodesets/Opc.Ua.IA.NodeSet2.xml"));
    QVERIFY(controller.importNodeSet(di));
    QVERIFY(controller.importNodeSet(ia));
    QCOMPARE(controller.modelNamespaceUris(di), QStringList { QStringLiteral("http://opcfoundation.org/UA/DI/") });
    QCOMPARE(controller.modelNamespaceUris(ia), QStringList { QStringLiteral("http://opcfoundation.org/UA/IA/") });
}

void TestNodeSets::modelVariableOfUnsupportedTypeIsReadOnly()
{
    ServerController controller;
    controller.setProject(tankProject());
    const VariableInfo recipe = modelVariable(controller, QStringLiteral("Recipe"));
    QVERIFY(recipe.fromModel);
    QVERIFY(recipe.readOnly);
    QVERIFY(!modelVariable(controller, QStringLiteral("Level")).readOnly);

    QString error;
    QVERIFY(!controller.updateVariable(recipe, &error));
    QVERIFY(error.contains(recipe.nodeId));
    QVERIFY(controller.project().modelVariables.isEmpty());
}

void TestNodeSets::modelEnumerationIsEditedAsInt32()
{
    ServerController controller;
    controller.setProject(tankProject());
    VariableInfo mode = modelVariable(controller, QStringLiteral("Mode"));
    QCOMPARE(mode.dataType, QStringLiteral("Int32"));
    QCOMPARE(mode.dataTypeName, QStringLiteral("TankMode"));
    QCOMPARE(mode.shownDataType(), QStringLiteral("TankMode"));
    QVERIFY(!mode.readOnly);
    QCOMPARE(modelVariable(controller, QStringLiteral("Recipe")).shownDataType(), QStringLiteral("NodeId"));
    QVERIFY(modelVariable(controller, QStringLiteral("Level")).dataTypeName.isEmpty());

    mode.initialValue = QStringLiteral("2");
    QString error;
    QVERIFY2(controller.updateVariable(mode, &error), qPrintable(error));
    QCOMPARE(controller.project().modelVariables.first().variable.dataType, QStringLiteral("Int32"));

    QVERIFY(controller.start());
    UaTestClient client;
    QCOMPARE(client.connect(controller.server().endpointUrl()), UA_STATUSCODE_GOOD);
    QVariant value;
    QCOMPARE(client.readValue(mode.nodeId, value), UA_STATUSCODE_GOOD);
    QCOMPARE(value.toInt(), 2);
}

void TestNodeSets::modelTypesAreListed()
{
    ServerController controller;
    controller.setProject(tankProject());
    const QHash<QString, TypeInfo> types = controller.snapshot().types;
    const TypeInfo mode = types.value(QStringLiteral("ns=2;i=3000"));
    QCOMPARE(mode.browseName, QStringLiteral("TankMode"));
    QCOMPARE(mode.baseTypeId, QStringLiteral("i=29"));
    QCOMPARE(mode.category, QStringLiteral("DataType"));
    QVERIFY(TypeCatalog::isRelevant(mode));

    const QList<TreeNode> tree = TypeCatalog::tree(types, false);
    const TreeNode *enumeration = findTreeNode(tree, QStringLiteral("i=29"));
    QVERIFY(enumeration);
    QVERIFY(std::any_of(enumeration->children.cbegin(), enumeration->children.cend(),
                        [&mode](const TreeNode &child) { return child.id == mode.nodeId; }));
}

void TestNodeSets::updateModelVariableKeepsSettings()
{
    ServerController controller;
    controller.setProject(tankProject());
    QSignalSpy edited(&controller, &ServerController::projectEdited);
    QSignalSpy changed(&controller, &ServerController::variableChanged);
    VariableInfo level = modelVariable(controller, QStringLiteral("Level"));
    level.displayName = QStringLiteral("Renamed");
    level.dataType = QStringLiteral("Int32");
    level.arrayLength = 3;
    level.icon = QStringLiteral("thermometer");
    level.access = QStringLiteral("Read/Write");
    level.configuredQuality = VariableInfo::Quality::Uncertain;
    level.initialValue = QStringLiteral("12.5");
    level.simulation.function = SimulationFunction::Sine;

    QString error;
    QVERIFY2(controller.updateVariable(level, &error), qPrintable(error));
    QCOMPARE(edited.count(), 1);
    QCOMPARE(changed.count(), 1);
    QCOMPARE(controller.project().modelVariables.size(), 1);
    const ModelVariable &stored = controller.project().modelVariables.first();
    QCOMPARE(stored.namespaceUri, kTankUri);
    QCOMPARE(stored.variable.nodeId, QStringLiteral("i=1001"));
    QCOMPARE(stored.variable.browseName, QStringLiteral("Level"));
    QCOMPARE(stored.variable.dataType, QStringLiteral("Double"));
    QCOMPARE(stored.variable.arrayLength, 0);
    QCOMPARE(stored.variable.icon, QStringLiteral("thermometer"));
    QCOMPARE(stored.variable.access, QStringLiteral("Read/Write"));
    QCOMPARE(stored.variable.quality, VariableInfo::Quality::Uncertain);
    QCOMPARE(stored.variable.initialValue.toDouble(), 12.5);
    QCOMPARE(stored.variable.simulation.function, SimulationFunction::Sine);

    const VariableInfo shown = modelVariable(controller, QStringLiteral("Level"));
    QCOMPARE(shown.nodeId, level.nodeId);
    QCOMPARE(shown.dataType, QStringLiteral("Double"));
    QCOMPARE(shown.value.toDouble(), 12.5);
    QCOMPARE(shown.quality, VariableInfo::Quality::Uncertain);
    QCOMPARE(shown.access, QStringLiteral("Read/Write"));
    QCOMPARE(shown.simulation.function, SimulationFunction::Sine);
    QVERIFY(shown.fromModel);
    level.icon.clear();
    QVERIFY2(controller.updateVariable(level, &error), qPrintable(error));
    QVERIFY(controller.project().modelVariables.first().variable.icon.isEmpty());
    QVERIFY(modelVariable(controller, QStringLiteral("Level")).icon.isEmpty());
    const ProjectConfig project = controller.project();
    controller.setProject(project);
    QVERIFY(modelVariable(controller, QStringLiteral("Level")).icon.isEmpty());
}

void TestNodeSets::updateModelVariableReplacesSettings()
{
    ServerController controller;
    controller.setProject(tankProject());
    VariableInfo level = modelVariable(controller, QStringLiteral("Level"));
    level.initialValue = QStringLiteral("12.5");
    QVERIFY(controller.updateVariable(level));

    level = modelVariable(controller, QStringLiteral("Level"));
    level.access = QStringLiteral("Write");
    level.initialValue = QVariant();
    QVERIFY(controller.updateVariable(level));
    QCOMPARE(controller.project().modelVariables.size(), 1);
    QCOMPARE(controller.project().modelVariables.first().variable.access, QStringLiteral("Write"));
    QCOMPARE(controller.project().modelVariables.first().variable.initialValue.toDouble(), 12.5);
}

void TestNodeSets::updateModelVariableRejectsInvalidValue()
{
    ServerController controller;
    controller.setProject(tankProject());
    VariableInfo level = modelVariable(controller, QStringLiteral("Level"));
    level.initialValue = QStringLiteral("full");

    QString error;
    QVERIFY(!controller.updateVariable(level, &error));
    QCOMPARE(error, QStringLiteral("'full' is not a Double value"));
    QVERIFY(controller.project().modelVariables.isEmpty());
    QCOMPARE(modelVariable(controller, QStringLiteral("Level")).value.toDouble(), 40.0);
}

void TestNodeSets::modelVariableRaisesAlarm()
{
    ServerController controller;
    controller.setProject(tankProject());
    VariableInfo level = modelVariable(controller, QStringLiteral("Level"));
    level.alarm.enabled = true;
    level.alarm.highEnabled = true;
    level.alarm.high = 30.0;

    QVERIFY(controller.updateVariable(level));
    QCOMPARE(modelVariable(controller, QStringLiteral("Level")).alarmState, QStringLiteral("High"));
    QVERIFY(logContains(controller.log(), QStringLiteral("Alarm of Level is active: High")));
}

void TestNodeSets::modelVariableIsSimulated()
{
    ServerController controller;
    controller.setProject(tankProject());
    QVERIFY(controller.start());
    QSignalSpy values(&controller, &ServerController::valuesChanged);
    VariableInfo level = modelVariable(controller, QStringLiteral("Level"));
    level.simulation.function = SimulationFunction::Ramp;
    level.simulation.intervalMs = 20;

    QVERIFY(controller.updateVariable(level));
    QTRY_VERIFY(std::any_of(values.cbegin(), values.cend(), [&level](const QList<QVariant> &arguments) {
        return arguments.first().value<QHash<QString, QVariant>>().contains(level.nodeId);
    }));
}

void TestNodeSets::modelSettingsApplyWhenProjectLoads()
{
    ServerController editor;
    editor.setProject(tankProject());
    VariableInfo level = modelVariable(editor, QStringLiteral("Level"));
    level.initialValue = QStringLiteral("12.5");
    level.configuredQuality = VariableInfo::Quality::Bad;
    QVERIFY(editor.updateVariable(level));

    ServerController controller;
    controller.setProject(editor.project());
    const VariableInfo loaded = modelVariable(controller, QStringLiteral("Level"));
    QCOMPARE(loaded.value.toDouble(), 12.5);
    QCOMPARE(loaded.quality, VariableInfo::Quality::Bad);
    QVERIFY(controller.addressSpace().hasVariable(loaded.nodeId));
}

void TestNodeSets::modelSettingsOfUnknownNamespaceAreLogged()
{
    ModelVariable orphan;
    orphan.namespaceUri = QStringLiteral("urn:missing");
    orphan.variable.kind = NodeDefinition::Kind::Variable;
    orphan.variable.browseName = QStringLiteral("Lost");
    orphan.variable.nodeId = QStringLiteral("i=1");
    ProjectConfig project = tankProject();
    project.modelVariables = { orphan };
    ModelVariable unknownNode = orphan;
    unknownNode.namespaceUri = kTankUri;
    unknownNode.variable.nodeId = QStringLiteral("i=9999");
    project.modelVariables.append(unknownNode);

    ServerController controller;
    controller.setProject(project);
    QVERIFY(logContains(controller.log(), QStringLiteral("urn:missing;i=1: no imported model has the namespace")));
    QVERIFY(logContains(controller.log(), QStringLiteral(";i=9999: the model has no such variable")));
    QCOMPARE(controller.project().modelVariables.size(), 2);
}

void TestNodeSets::removeNodeSetDropsModelSettings()
{
    ProjectConfig project = tankProject();
    project.nodeSetFiles.append(testData(QStringLiteral("mixer.NodeSet2.xml")));
    ServerController controller;
    controller.setProject(project);
    VariableInfo level = modelVariable(controller, QStringLiteral("Level"));
    level.access = QStringLiteral("Read/Write");
    QVERIFY(controller.updateVariable(level));
    VariableInfo speed = modelVariable(controller, QStringLiteral("Speed"));
    speed.access = QStringLiteral("Read");
    QVERIFY(controller.updateVariable(speed));

    QVERIFY(controller.removeNodeSet(project.nodeSetFiles.first()));
    QCOMPARE(controller.project().modelVariables.size(), 1);
    QCOMPARE(controller.project().modelVariables.first().namespaceUri, QStringLiteral("urn:openuasim:test:mixer"));
}

///
/// \brief Keeps summary and snapshot node totals consistent across model lifecycle operations.
///
void TestNodeSets::totalNodesTracksImportedModels()
{
    ServerController controller;
    controller.setProject(TestProject::create());
    const int initialCount = controller.status().totalNodes;
    QCOMPARE(initialCount, 17);
    const QString tank = testData(QStringLiteral("tank.NodeSet2.xml"));
    QString error;
    QVERIFY2(controller.importNodeSet(tank, &error), qPrintable(error));
    QCOMPARE(controller.status().totalNodes, initialCount + 5);
    QCOMPARE(controller.snapshot().status.totalNodes, initialCount + 5);

    QVERIFY(!controller.importNodeSet(tank, &error));
    QVERIFY(!controller.importNodeSet(testData(QStringLiteral("missing.xml")), &error));
    QCOMPARE(controller.status().totalNodes, initialCount + 5);
    QCOMPARE(controller.snapshot().status.totalNodes, initialCount + 5);

    const ProjectConfig imported = controller.project();
    controller.setProject(imported);
    QCOMPARE(controller.status().totalNodes, initialCount + 5);
    const QString pump = testData(QStringLiteral("pump.NodeSet2.xml"));
    QVERIFY2(controller.importNodeSet(pump, &error), qPrintable(error));
    QCOMPARE(controller.status().totalNodes, initialCount + 5 + 11);
    QCOMPARE(controller.snapshot().status.totalNodes, initialCount + 5 + 11);
    QVERIFY2(controller.removeNodeSet(pump, &error), qPrintable(error));
    QCOMPARE(controller.status().totalNodes, initialCount + 5);
    QCOMPARE(controller.snapshot().status.totalNodes, initialCount + 5);
    QVERIFY2(controller.removeNodeSet(tank, &error), qPrintable(error));
    QCOMPARE(controller.status().totalNodes, initialCount);
    QCOMPARE(controller.snapshot().status.totalNodes, initialCount);
    QVERIFY(!controller.removeNodeSet(tank, &error));
    QCOMPARE(controller.status().totalNodes, initialCount);

    controller.setProject(imported);
    QCOMPARE(controller.status().totalNodes, initialCount + 5);
    QCOMPARE(controller.snapshot().status.totalNodes, initialCount + 5);
    controller.setProject(ProjectConfig());
    QCOMPARE(controller.status().totalNodes, 0);
    QCOMPARE(controller.snapshot().status.totalNodes, 0);
}

QTEST_GUILESS_MAIN(TestNodeSets)

#include "test_nodesets.moc"
