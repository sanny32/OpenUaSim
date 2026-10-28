// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <QUaServer>

#include "server/nodesetloader.h"
#include "server/simserver.h"

namespace {

QString testData(const QString &name)
{
    return QStringLiteral(OUASIM_TEST_DATA_DIR "/") + name;
}

const QString kTankUri = QStringLiteral("urn:openuasim:test:tank");

QString publicModel(const QString &name)
{
    return testData(QStringLiteral("public-nodesets/") + name);
}

const QString kBrokenUri = QStringLiteral("urn:openuasim:test:broken");

///
/// \brief Writes a model in the broken namespace with one object, typed by a missing type unless \a valid.
///
QString writeBrokenNamespaceModel(const QTemporaryDir &directory, const QString &name, bool valid)
{
    const QString type = valid ? QStringLiteral("i=58") : QStringLiteral("ns=1;i=9999");
    const QString fileName = directory.filePath(name);
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
        return {};
    file.write(QStringLiteral(
                   "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                   "<UANodeSet xmlns=\"http://opcfoundation.org/UA/2011/03/UANodeSet.xsd\">\n"
                   "  <NamespaceUris><Uri>%1</Uri></NamespaceUris>\n"
                   "  <UAObject NodeId=\"ns=1;i=1000\" BrowseName=\"1:Object\">\n"
                   "    <DisplayName>Object</DisplayName>\n"
                   "    <References>\n"
                   "      <Reference ReferenceType=\"i=35\" IsForward=\"false\">i=85</Reference>\n"
                   "      <Reference ReferenceType=\"i=40\">%2</Reference>\n"
                   "    </References>\n"
                   "  </UAObject>\n"
                   "</UANodeSet>\n")
                   .arg(kBrokenUri, type)
                   .toUtf8());
    return fileName;
}

///
/// \brief Writes a model whose Models section requires the broken namespace.
///
QString writeDependentModel(const QTemporaryDir &directory)
{
    const QString fileName = directory.filePath(QStringLiteral("dependent.xml"));
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
        return {};
    file.write(QStringLiteral(
                   "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                   "<UANodeSet xmlns=\"http://opcfoundation.org/UA/2011/03/UANodeSet.xsd\">\n"
                   "  <NamespaceUris><Uri>urn:openuasim:test:dependent</Uri><Uri>%1</Uri></NamespaceUris>\n"
                   "  <Models>\n"
                   "    <Model ModelUri=\"urn:openuasim:test:dependent\">\n"
                   "      <RequiredModel ModelUri=\"http://opcfoundation.org/UA/\" />\n"
                   "      <RequiredModel ModelUri=\"%1\" />\n"
                   "    </Model>\n"
                   "  </Models>\n"
                   "  <UAObject NodeId=\"ns=1;i=1000\" BrowseName=\"1:Dependent\">\n"
                   "    <DisplayName>Dependent</DisplayName>\n"
                   "    <References>\n"
                   "      <Reference ReferenceType=\"i=35\" IsForward=\"false\">i=85</Reference>\n"
                   "      <Reference ReferenceType=\"i=40\">i=58</Reference>\n"
                   "    </References>\n"
                   "  </UAObject>\n"
                   "</UANodeSet>\n")
                   .arg(kBrokenUri)
                   .toUtf8());
    return fileName;
}

bool hasLevel(const QSignalSpy &spy, LogEntry::Level level)
{
    for (const QList<QVariant> &arguments : spy) {
        if (arguments.first().value<LogEntry>().level == level)
            return true;
    }
    return false;
}

}

class TestNodeSetLoader : public QObject
{
    Q_OBJECT

private slots:
    void loadsModelOnce();
    void reportsUnreadableModel();
    void refusesInstanceModelWithoutItsNamespaces();
    void refusesModelWithoutRequiredModels();
    void loadsModelAfterRequiredModels();
    void refusesModelWithoutAddedNodes();
    void mapsNewNamespacesToModel();
    void renamesLoadedFiles();
    void clearForgetsModels();
    void exportsNamespaces();
    void refusesStandardNamespace();
};

void TestNodeSetLoader::loadsModelOnce()
{
    SimServer server;
    NodeSetLoader loader(server);
    QSignalSpy log(&loader, &NodeSetLoader::logAppended);
    const QString tank = testData(QStringLiteral("tank.NodeSet2.xml"));

    QVERIFY(loader.load(tank));
    QCOMPARE(loader.loaded(), QStringList({ tank }));
    QVERIFY(hasLevel(log, LogEntry::Level::Info));
    const qsizetype entries = log.count();

    QVERIFY(loader.load(tank));
    loader.loadAll({ tank });
    QCOMPARE(loader.loaded(), QStringList({ tank }));
    QCOMPARE(log.count(), entries);
}

void TestNodeSetLoader::reportsUnreadableModel()
{
    SimServer server;
    NodeSetLoader loader(server);
    QSignalSpy log(&loader, &NodeSetLoader::logAppended);

    QString error;
    QVERIFY(!loader.load(testData(QStringLiteral("missing.NodeSet2.xml")), &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(loader.loaded().isEmpty());
    QVERIFY(hasLevel(log, LogEntry::Level::Error));
}

void TestNodeSetLoader::refusesInstanceModelWithoutItsNamespaces()
{
    SimServer server;
    NodeSetLoader loader(server);
    QSignalSpy log(&loader, &NodeSetLoader::logAppended);

    QString error;
    QVERIFY(!loader.load(publicModel(QStringLiteral("instanceexample.xml")), &error));
    QVERIFY2(error.contains(QLatin1String("http://opcfoundation.org/UA/Pumps/")), qPrintable(error));
    QVERIFY2(error.contains(QLatin1String("http://opcfoundation.org/UA/Machinery/")), qPrintable(error));
    QVERIFY2(error.contains(QLatin1String("http://opcfoundation.org/UA/DI/")), qPrintable(error));
    QVERIFY(!error.contains(QLatin1String("InstanceExample")));
    QVERIFY(loader.loaded().isEmpty());
    QVERIFY(!server.uaServer()->namespaces().contains(QStringLiteral("http://yourorganisation.org/InstanceExample/")));
    QCOMPARE(log.count(), 1);
    QVERIFY(hasLevel(log, LogEntry::Level::Error));
}

void TestNodeSetLoader::refusesModelWithoutRequiredModels()
{
    SimServer server;
    NodeSetLoader loader(server);

    QString error;
    QVERIFY(!loader.load(publicModel(QStringLiteral("Opc.Ua.Pumps.NodeSet2.xml")), &error));
    QVERIFY2(error.contains(QLatin1String("http://opcfoundation.org/UA/Machinery/")), qPrintable(error));
    QVERIFY2(error.contains(QLatin1String("http://opcfoundation.org/UA/DI/")), qPrintable(error));
    QVERIFY2(!error.contains(QLatin1String("Pumps")), qPrintable(error));
    QVERIFY(!server.uaServer()->namespaces().contains(QStringLiteral("http://opcfoundation.org/UA/Pumps/")));
}

void TestNodeSetLoader::loadsModelAfterRequiredModels()
{
    SimServer server;
    NodeSetLoader loader(server);
    const QString instance = publicModel(QStringLiteral("instanceexample.xml"));
    QVERIFY(!loader.load(instance));

    for (const QString &model : { QStringLiteral("Opc.Ua.Di.NodeSet2.xml"), QStringLiteral("Opc.Ua.IA.NodeSet2.xml"),
                                  QStringLiteral("Opc.Ua.Machinery.NodeSet2.xml"),
                                  QStringLiteral("Opc.Ua.Pumps.NodeSet2.xml") }) {
        QString error;
        QVERIFY2(loader.load(publicModel(model), &error), qPrintable(model + QStringLiteral(": ") + error));
    }
    QString error;
    QVERIFY2(loader.load(instance, &error), qPrintable(error));
    QCOMPARE(loader.modelNamespaces().value(QStringLiteral("http://yourorganisation.org/InstanceExample/")), instance);
}

void TestNodeSetLoader::refusesModelWithoutAddedNodes()
{
    SimServer server;
    NodeSetLoader loader(server);
    QSignalSpy log(&loader, &NodeSetLoader::logAppended);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString broken = writeBrokenNamespaceModel(directory, QStringLiteral("broken.xml"), false);
    const QString dependent = writeDependentModel(directory);

    QString error;
    QVERIFY(!loader.load(broken, &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(loader.loaded().isEmpty());
    QVERIFY(!loader.modelNamespaces().contains(kBrokenUri));
    QVERIFY(hasLevel(log, LogEntry::Level::Error));
    QVERIFY(!hasLevel(log, LogEntry::Level::Info));

    QVERIFY2(!loader.load(dependent, &error), "an empty namespace left by a failed load is not a loaded model");
    QVERIFY2(error.contains(kBrokenUri), qPrintable(error));

    const QString fixed = writeBrokenNamespaceModel(directory, QStringLiteral("fixed.xml"), true);
    QVERIFY2(loader.load(fixed, &error), qPrintable(error));
    QCOMPARE(loader.modelNamespaces().value(kBrokenUri), fixed);
    QVERIFY2(loader.load(dependent, &error), qPrintable(error));
}

void TestNodeSetLoader::mapsNewNamespacesToModel()
{
    SimServer server;
    NodeSetLoader loader(server);
    const QString tank = testData(QStringLiteral("tank.NodeSet2.xml"));
    QVERIFY(loader.load(tank));

    const QHash<QString, QString> namespaces = loader.modelNamespaces();
    QCOMPARE(namespaces.value(kTankUri), tank);
    QVERIFY(!namespaces.contains(server.uaServer()->namespaces().value(0)));
    QVERIFY(!namespaces.contains(server.uaServer()->namespaces().value(1)));
}

void TestNodeSetLoader::renamesLoadedFiles()
{
    SimServer server;
    NodeSetLoader loader(server);
    const QString tank = testData(QStringLiteral("tank.NodeSet2.xml"));
    QVERIFY(loader.load(tank));

    const QString moved = QStringLiteral("/moved/tank.NodeSet2.xml");
    loader.renameFiles({ { tank, moved }, { QStringLiteral("/other.xml"), QStringLiteral("/elsewhere.xml") } });
    QCOMPARE(loader.loaded(), QStringList({ moved }));
    QCOMPARE(loader.modelNamespaces().value(kTankUri), moved);
}

void TestNodeSetLoader::clearForgetsModels()
{
    SimServer server;
    NodeSetLoader loader(server);
    const QString tank = testData(QStringLiteral("tank.NodeSet2.xml"));
    QVERIFY(loader.load(tank));

    loader.clear();
    QVERIFY(loader.loaded().isEmpty());
    QVERIFY(loader.modelNamespaces().isEmpty());

    server.reset();
    QVERIFY(loader.load(tank));
    QCOMPARE(loader.modelNamespaces().value(kTankUri), tank);
}

void TestNodeSetLoader::exportsNamespaces()
{
    SimServer server;
    NodeSetLoader loader(server);
    QVERIFY(loader.load(testData(QStringLiteral("tank.NodeSet2.xml"))));
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString model = directory.filePath(QStringLiteral("tank.xml"));
    QString error;
    QVERIFY2(loader.exportNamespace(model, kTankUri, &error), qPrintable(error));
    QFile exported(model);
    QVERIFY(exported.open(QIODevice::ReadOnly));
    QVERIFY(exported.readAll().contains(kTankUri.toUtf8()));

    const QString application = directory.filePath(QStringLiteral("application.xml"));
    QVERIFY2(loader.exportApplicationNamespace(application, &error), qPrintable(error));
    QVERIFY(QFile::exists(application));
}

void TestNodeSetLoader::refusesStandardNamespace()
{
    SimServer server;
    NodeSetLoader loader(server);
    QTemporaryDir directory;
    const QString fileName = directory.filePath(QStringLiteral("ns0.xml"));

    QString error;
    QVERIFY(!loader.exportNamespace(fileName, server.uaServer()->namespaces().value(0), &error));
    QVERIFY(!error.isEmpty());
    error.clear();
    QVERIFY(!loader.exportNamespace(fileName, QString(), &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!QFile::exists(fileName));
}

QTEST_GUILESS_MAIN(TestNodeSetLoader)
#include "test_nodesetloader.moc"
