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
