// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "pki/certificatestore.h"
#include "project/defaultproject.h"
#include "server/servercontroller.h"
#include "testport.h"
#include "uatestclient.h"

namespace {

QString testData(const QString &name)
{
    return QStringLiteral(OUASIM_TEST_DATA_DIR "/pki/") + name;
}

using Category = CertificateInfo::Category;

}

class TestCertificateStore : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void emptyStoreListsNothing();
    void importsPemAndDer();
    void describesCertificate();
    void importRejectsNonCertificates();
    void importRefusesServerFolder();
    void installsServerCertificate();
    void installRejectsMissingKey();
    void movesBetweenFolders();
    void moveRejectsFilesOutsideStore();
    void removesCertificates();
    void removingServerCertificateRemovesKey();
    void skipsGarbageFiles();
    void materialCollectsFolders();
    void controllerAnnouncesChanges();
    void secureEndpointRunsWithInstalledCertificate();

private:
    QTemporaryDir *_directory = nullptr;
    CertificateStore _store;
};

void TestCertificateStore::init()
{
    delete _directory;
    _directory = new QTemporaryDir;
    QVERIFY(_directory->isValid());
    _store.setRoot(_directory->path());
}

void TestCertificateStore::emptyStoreListsNothing()
{
    QVERIFY(_store.certificates().isEmpty());
    const PkiMaterial material = _store.material();
    QVERIFY(material.certificate.isEmpty());
    QVERIFY(material.trusted.isEmpty());
}

void TestCertificateStore::importsPemAndDer()
{
    QVERIFY(!_store.importCertificate(testData(QStringLiteral("server.cert.pem")), Category::Trusted).isEmpty());
    const QString stored = _store.importCertificate(testData(QStringLiteral("client.der")), Category::Issuer);
    QVERIFY(QFileInfo::exists(stored));
    QVERIFY(QFileInfo(stored).fileName().startsWith(QStringLiteral("Test Client [")));

    const QList<CertificateInfo> certificates = _store.certificates();
    QCOMPARE(certificates.size(), 2);
    QCOMPARE(certificates[0].category, Category::Trusted);
    QCOMPARE(certificates[1].category, Category::Issuer);
}

void TestCertificateStore::describesCertificate()
{
    _store.importCertificate(testData(QStringLiteral("server.cert.pem")), Category::Trusted);
    const CertificateInfo info = _store.certificates().first();
    QCOMPARE(info.commonName, QStringLiteral("OpenUaSim Test Server"));
    QCOMPARE(info.subject, QStringLiteral("CN=OpenUaSim Test Server,\nO=OpenUaSim,\nC=US"));
    QCOMPARE(info.issuer, info.subject);
    QCOMPARE(info.fingerprint, QStringLiteral("35:46:E2:02:01:BF:3E:CD:F2:C4:D6:55:93:C7:48:66:AE:49:1B:70"));
    QVERIFY(!info.serialNumber.isEmpty());
    QVERIFY(!info.serialNumber.contains(QLatin1Char(':')));
    QVERIFY(info.validFrom < info.validTo);
    QCOMPARE(info.validTo.date().year(), 2126);
    QVERIFY(QFileInfo::exists(info.filePath));
}

void TestCertificateStore::importRejectsNonCertificates()
{
    QString error;
    QVERIFY(_store.importCertificate(testData(QStringLiteral("server.key.pem")), Category::Trusted, &error).isEmpty());
    QVERIFY(error.contains(QStringLiteral("is not a PEM or DER certificate")));
    QVERIFY(_store.importCertificate(testData(QStringLiteral("missing.der")), Category::Trusted).isEmpty());
    QVERIFY(_store.certificates().isEmpty());
}

void TestCertificateStore::importRefusesServerFolder()
{
    QString error;
    QVERIFY(_store.importCertificate(testData(QStringLiteral("client.der")), Category::Application, &error).isEmpty());
    QVERIFY(error.contains(QStringLiteral("private key")));
}

void TestCertificateStore::installsServerCertificate()
{
    QVERIFY(_store.installServerCertificate(testData(QStringLiteral("server.cert.pem")),
                                            testData(QStringLiteral("server.key.pem"))));
    const QList<CertificateInfo> certificates = _store.certificates();
    QCOMPARE(certificates.size(), 1);
    QCOMPARE(certificates.first().category, Category::Application);

    const PkiMaterial material = _store.material();
    QVERIFY(!material.certificate.isEmpty());
    QVERIFY(material.privateKey.contains("PRIVATE KEY"));
}

void TestCertificateStore::installRejectsMissingKey()
{
    QString error;
    QVERIFY(!_store.installServerCertificate(testData(QStringLiteral("server.cert.pem")),
                                             testData(QStringLiteral("missing.pem")), &error));
    QVERIFY(error.contains(QStringLiteral("holds no private key")));
    QVERIFY(!_store.installServerCertificate(testData(QStringLiteral("server.key.pem")),
                                             testData(QStringLiteral("server.key.pem")), &error));
    QVERIFY(_store.certificates().isEmpty());
}

void TestCertificateStore::movesBetweenFolders()
{
    const QString rejected = _store.importCertificate(testData(QStringLiteral("client.der")), Category::Rejected);
    const QString trusted = _store.move(rejected, Category::Trusted);
    QVERIFY(!trusted.isEmpty());
    QVERIFY(!QFileInfo::exists(rejected));
    QCOMPARE(_store.certificates().first().category, Category::Trusted);

    QVERIFY(!_store.move(trusted, Category::Rejected).isEmpty());
    QCOMPARE(_store.certificates().first().category, Category::Rejected);
}

void TestCertificateStore::moveRejectsFilesOutsideStore()
{
    QString error;
    QVERIFY(_store.move(testData(QStringLiteral("client.der")), Category::Trusted, &error).isEmpty());
    QVERIFY(!error.isEmpty());
    QVERIFY(QFileInfo::exists(testData(QStringLiteral("client.der"))));
    const QString stored = _store.importCertificate(testData(QStringLiteral("client.der")), Category::Trusted);
    QVERIFY(_store.move(stored, Category::Application).isEmpty());
}

void TestCertificateStore::removesCertificates()
{
    const QString stored = _store.importCertificate(testData(QStringLiteral("client.der")), Category::Trusted);
    QVERIFY(_store.remove(stored));
    QVERIFY(_store.certificates().isEmpty());
    QString error;
    QVERIFY(!_store.remove(stored, &error));
    QVERIFY(!_store.remove(testData(QStringLiteral("client.der"))));
    QVERIFY(QFileInfo::exists(testData(QStringLiteral("client.der"))));
}

void TestCertificateStore::removingServerCertificateRemovesKey()
{
    QVERIFY(_store.installServerCertificate(testData(QStringLiteral("server.cert.pem")),
                                            testData(QStringLiteral("server.key.pem"))));
    QVERIFY(_store.remove(_store.certificates().first().filePath));
    QVERIFY(_store.material().privateKey.isEmpty());
}

void TestCertificateStore::skipsGarbageFiles()
{
    QDir().mkpath(_store.folder(Category::Trusted));
    QFile garbage(_store.folder(Category::Trusted) + QStringLiteral("/notes.txt"));
    QVERIFY(garbage.open(QIODevice::WriteOnly));
    garbage.write("not a certificate");
    garbage.close();
    QVERIFY(_store.certificates().isEmpty());
}

void TestCertificateStore::materialCollectsFolders()
{
    _store.importCertificate(testData(QStringLiteral("client.der")), Category::Trusted);
    _store.importCertificate(testData(QStringLiteral("server.cert.pem")), Category::Issuer);
    _store.importCertificate(testData(QStringLiteral("server.cert.pem")), Category::Rejected);
    const PkiMaterial material = _store.material();
    QCOMPARE(material.trusted.size(), 1);
    QCOMPARE(material.issuers.size(), 1);
}

void TestCertificateStore::controllerAnnouncesChanges()
{
    ProjectConfig project = DefaultProject::create();
    project.endpoints.first().port = TestPort::freePort();
    ServerController controller;
    controller.certificateStore().setRoot(_directory->path());
    controller.setProject(project);
    QVERIFY(controller.start());
    QSignalSpy changed(&controller, &ServerController::certificatesChanged);

    QVERIFY(controller.importCertificate(testData(QStringLiteral("client.der")), Category::Rejected));
    QCOMPARE(changed.count(), 1);
    QVERIFY(controller.isRestartRequired());
    QCOMPARE(controller.snapshot().certificates.size(), 1);

    const QString stored = controller.certificates().first().filePath;
    QVERIFY(controller.moveCertificate(stored, Category::Trusted));
    QVERIFY(controller.removeCertificate(controller.certificates().first().filePath));
    QCOMPARE(changed.count(), 3);

    QString error;
    QVERIFY(!controller.removeCertificate(stored, &error));
    QCOMPARE(changed.count(), 3);
}

void TestCertificateStore::secureEndpointRunsWithInstalledCertificate()
{
#ifndef OUASIM_HAVE_ENCRYPTION
    QSKIP("Built without OUASIM_WITH_ENCRYPTION");
#else
    ProjectConfig project = DefaultProject::create();
    EndpointInfo &endpoint = project.endpoints.first();
    endpoint.port = TestPort::freePort();
    endpoint.securityPolicy = QStringLiteral("Basic256Sha256");
    endpoint.securityMode = QStringLiteral("Sign & Encrypt");

    ServerController controller;
    controller.certificateStore().setRoot(_directory->path());
    controller.setProject(project);
    QVERIFY(!controller.start());

    QVERIFY(controller.installServerCertificate(testData(QStringLiteral("server.cert.pem")),
                                                testData(QStringLiteral("server.key.pem"))));
    QVERIFY(controller.start());
#endif
}

QTEST_GUILESS_MAIN(TestCertificateStore)

#include "test_certificatestore.moc"
