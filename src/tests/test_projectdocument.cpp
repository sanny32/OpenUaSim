// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QSysInfo>
#include <QTest>

#include <QUaServer>

#include "project/projectdocument.h"
#include "project/projectfile.h"
#include "server/listenersettings.h"
#include "server/servercontroller.h"
#include "testport.h"
#include "testproject.h"

class TestProjectDocument : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void newProjectServesDefault();
    void newProjectOffersEveryEndpoint();
    void editsMarkModified();
    void saveWithoutFileFails();
    void saveAsWritesAndClearsModified();
    void saveAsKeepsModelsNextToProject();
    void saveAsReportsUncopyableModel();
    void openServesFileAndClearsModified();
    void openKeepsProjectOnError();
    void openRestartsRunningServer();
    void openLastReopensSavedFile();
    void openLastFallsBackToDefault();

private:
    QTemporaryDir _directory;
};

void TestProjectDocument::init()
{
    QSettings().clear();
}

void TestProjectDocument::newProjectServesDefault()
{
    ServerController controller;
    ProjectDocument document(&controller);
    document.newProject();
    QCOMPARE(controller.project().nodes.size(), 1);
    QVERIFY(document.fileName().isEmpty());
    QCOMPARE(document.displayName(), QStringLiteral("Untitled"));
    QVERIFY(!document.isModified());
}

void TestProjectDocument::newProjectOffersEveryEndpoint()
{
    ServerController controller;
    ProjectDocument document(&controller);
    document.newProject();
    const QList<EndpointInfo> &endpoints = controller.project().endpoints;

    QSet<QString> pairs;
    for (const EndpointInfo &endpoint : endpoints) {
        QVERIFY(endpoint.enabled);
        QVERIFY(endpoint.allowAnonymous);
        QCOMPARE(endpoint.hostname, QSysInfo::machineHostName());
        QCOMPARE(endpoint.url(), QStringLiteral("opc.tcp://%1:48400").arg(QSysInfo::machineHostName()));
        pairs.insert(endpoint.securityPolicy + QLatin1Char('/') + endpoint.securityMode);
    }
    QCOMPARE(endpoints.size(), 11);
    QCOMPARE(pairs.size(), endpoints.size());
    QCOMPARE(endpoints.first().securityPolicy, QStringLiteral("None"));

    const ListenerSettings listener = ListenerSettingsBuilder::build(endpoints);
    QVERIFY2(listener.ok(), qPrintable(listener.error));
    QCOMPARE(listener.securityPolicies, ListenerSettingsBuilder::securityPolicyNames());
    QCOMPARE(listener.securityModes, ListenerSettingsBuilder::securityModeNames());
}

void TestProjectDocument::editsMarkModified()
{
    ServerController controller;
    ProjectDocument document(&controller);
    document.newProject();
    QSignalSpy modified(&document, &ProjectDocument::modifiedChanged);

    QVERIFY(controller.removeNode(QStringLiteral("ns=1;s=Pump_01")));
    QVERIFY(document.isModified());
    QCOMPARE(modified.count(), 1);
    QVERIFY(controller.removeNode(QStringLiteral("ns=1;s=Tank_01")));
    QCOMPARE(modified.count(), 1);

    document.newProject();
    QVERIFY(!document.isModified());
}

void TestProjectDocument::saveWithoutFileFails()
{
    ServerController controller;
    ProjectDocument document(&controller);
    document.newProject();
    QString error;
    QVERIFY(!document.save(&error));
    QVERIFY(!error.isEmpty());
}

void TestProjectDocument::saveAsWritesAndClearsModified()
{
    ServerController controller;
    ProjectDocument document(&controller);
    document.newProject();
    QVERIFY(controller.removeNode(QStringLiteral("ns=1;s=Pump_01")));
    QSignalSpy names(&document, &ProjectDocument::fileNameChanged);

    const QString fileName = _directory.filePath(QStringLiteral("line.ouasim"));
    QVERIFY(document.saveAs(fileName));
    QVERIFY(!document.isModified());
    QCOMPARE(document.fileName(), fileName);
    QCOMPARE(document.displayName(), QStringLiteral("line.ouasim"));
    QCOMPARE(names.count(), 1);

    const ProjectFile::LoadResult saved = ProjectFile::load(fileName);
    QVERIFY(saved.ok());
    QCOMPARE(saved.project.nodes.first().children.size(), 3);

    QVERIFY(controller.removeNode(QStringLiteral("ns=1;s=Tank_01")));
    QVERIFY(document.save());
    QCOMPARE(ProjectFile::load(fileName).project.nodes.first().children.size(), 2);
}

void TestProjectDocument::saveAsKeepsModelsNextToProject()
{
    const QString download = _directory.filePath(QStringLiteral("download/tank.NodeSet2.xml"));
    QVERIFY(QDir().mkpath(QFileInfo(download).absolutePath()));
    QVERIFY(QFile::copy(QStringLiteral(OUASIM_TEST_DATA_DIR "/tank.NodeSet2.xml"), download));
    ServerController controller;
    ProjectDocument document(&controller);
    document.newProject();
    QVERIFY(controller.importNodeSet(download));

    const QString fileName = _directory.filePath(QStringLiteral("plant.ouasim"));
    QString error;
    QVERIFY2(document.saveAs(fileName, &error), qPrintable(error));
    const QString copy = _directory.filePath(QStringLiteral("plant.models/tank.NodeSet2.xml"));
    QVERIFY(QFile::exists(copy));
    QCOMPARE(controller.project().nodeSetFiles, QStringList { copy });
    QCOMPARE(controller.modelNamespaces().value(QStringLiteral("urn:openuasim:test:tank")), copy);
    QVERIFY(!document.isModified());

    QVERIFY(QFile::remove(download));
    ServerController reopened;
    ProjectDocument other(&reopened);
    QVERIFY(other.open(fileName));
    QCOMPARE(reopened.project().nodeSetFiles, QStringList { copy });
    QVERIFY(reopened.server().uaServer()->namespaces().contains(QStringLiteral("urn:openuasim:test:tank")));
}

void TestProjectDocument::saveAsReportsUncopyableModel()
{
    ProjectConfig project = TestProject::create();
    project.nodeSetFiles = { _directory.filePath(QStringLiteral("missing.NodeSet2.xml")) };
    ServerController controller;
    ProjectDocument document(&controller);
    controller.setProject(project);

    const QString fileName = _directory.filePath(QStringLiteral("broken.ouasim"));
    QString error;
    QVERIFY(!document.saveAs(fileName, &error));
    QVERIFY(error.contains(QStringLiteral("missing.NodeSet2.xml")));
    QVERIFY(!QFile::exists(fileName));
    QVERIFY(document.fileName().isEmpty());
}

void TestProjectDocument::openServesFileAndClearsModified()
{
    ProjectConfig project = TestProject::create();
    project.nodes.first().browseName = QStringLiteral("Line_02");
    const QString fileName = _directory.filePath(QStringLiteral("other.ouasim"));
    QVERIFY(ProjectFile::save(project, fileName));

    ServerController controller;
    ProjectDocument document(&controller);
    document.newProject();
    QVERIFY(controller.removeNode(QStringLiteral("ns=1;s=Pump_01")));
    QVERIFY(document.open(fileName));
    QCOMPARE(controller.project().nodes.first().browseName, QStringLiteral("Line_02"));
    QVERIFY(!document.isModified());
    QCOMPARE(document.fileName(), fileName);
}

void TestProjectDocument::openKeepsProjectOnError()
{
    const QString fileName = _directory.filePath(QStringLiteral("broken.ouasim"));
    QFile file(fileName);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("{ broken");
    file.close();

    ServerController controller;
    ProjectDocument document(&controller);
    document.newProject();
    QString error;
    QVERIFY(!document.open(fileName, &error));
    QVERIFY(error.startsWith(QStringLiteral("Malformed project file")));
    QCOMPARE(controller.project().nodes.size(), 1);
    QVERIFY(document.fileName().isEmpty());
}

void TestProjectDocument::openRestartsRunningServer()
{
    ProjectConfig project = TestProject::create();
    project.endpoints.first().port = TestPort::freePort();
    const QString fileName = _directory.filePath(QStringLiteral("running.ouasim"));
    QVERIFY(ProjectFile::save(project, fileName));

    ServerController controller;
    ProjectDocument document(&controller);
    document.newProject();
    QList<EndpointInfo> unsecured = { controller.project().endpoints.first() };
    unsecured.first().port = TestPort::freePort();
    QVERIFY(controller.setEndpoints(unsecured));
    QVERIFY(controller.start());

    QVERIFY(document.open(fileName));
    QVERIFY(controller.isRunning());
    QCOMPARE(controller.server().endpointUrl(), project.endpoints.first().url());
}

void TestProjectDocument::openLastReopensSavedFile()
{
    const QString fileName = _directory.filePath(QStringLiteral("last.ouasim"));
    {
        ServerController controller;
        ProjectDocument document(&controller);
        document.newProject();
        QVERIFY(controller.removeNode(QStringLiteral("ns=1;s=Pump_01")));
        QVERIFY(document.saveAs(fileName));
    }
    ServerController controller;
    ProjectDocument document(&controller);
    QVERIFY(document.openLast());
    QCOMPARE(document.displayName(), QStringLiteral("last.ouasim"));
    QCOMPARE(controller.project().nodes.first().children.size(), 3);
}

void TestProjectDocument::openLastFallsBackToDefault()
{
    QSettings().setValue(QStringLiteral("project/lastFile"), _directory.filePath(QStringLiteral("gone.ouasim")));
    ServerController controller;
    ProjectDocument document(&controller);
    QVERIFY(!document.openLast());
    QCOMPARE(controller.project().nodes.size(), 1);
    QVERIFY(document.fileName().isEmpty());
}

QTEST_GUILESS_MAIN(TestProjectDocument)

#include "test_projectdocument.moc"
