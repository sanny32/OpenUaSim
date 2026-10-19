// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QBuffer>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QXmlStreamReader>

#include <QUaServer>

#include "project/defaultproject.h"
#include "server/servercontroller.h"

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
    void exportWritesFile();
    void exportReportsUnwritablePath();

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
    QVERIFY(logContains(controller.log(), QStringLiteral("stays loaded until OpenUaSim restarts")));
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

void TestNodeSets::exportReportsUnwritablePath()
{
    ServerController controller;
    QString error;
    QVERIFY(!controller.exportNodeSet(_directory.filePath(QStringLiteral("no/such/dir/x.xml")), &error));
    QVERIFY(!error.isEmpty());
}

QTEST_GUILESS_MAIN(TestNodeSets)

#include "test_nodesets.moc"
