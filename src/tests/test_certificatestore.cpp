// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHostInfo>
#include <QSignalSpy>
#include <QSslCertificate>
#include <QTemporaryDir>
#include <QTest>

#include "pki/certificategenerator.h"
#include "pki/certificatestore.h"
#include "server/servercontroller.h"
#include "testport.h"
#include "testproject.h"
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
    void storesServerCertificateFromMemory();
    void loadsServerCertificateFromAnotherHost();
    void serverCertificateWriteFailure();
    void generatesSelfSignedCertificate();
    void generatesWithLongSubjectNames();
    void movesBetweenFolders();
    void moveRejectsFilesOutsideStore();
    void removesCertificates();
    void removingServerCertificateRemovesKey();
    void skipsGarbageFiles();
    void materialCollectsFolders();
    void controllerAnnouncesChanges();
    void secureEndpointRunsWithInstalledCertificate();
    void startCreatesMissingServerCertificate();
    void userCertificateLogsInByCommonName_data();
    void userCertificateLogsInByCommonName();
    void defaultProjectStartsWithEveryEndpoint();
    void ensuresCertificateBeforeServerStart_data();
    void ensuresCertificateBeforeServerStart();
    void ensureKeepsInstalledCertificate();
    void ensureReportsGenerationFailure();
    void generateReplacesServerCertificate();
    void addsCertificateFromMemory();
    void findsStoredCertificateByContent();
    void rejectedClientLandsInRejectedFolder();
    void trustedClientsOnlyRejectsClientsWithEmptyTrustList();
    void anyClientConnectsWithoutTrustedClientsOnly();
    void unverifiedClientLandsInRejectedFolder();
    void unverifiedClientAlreadyStoredIsNotCopied();

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
    QFile file(testData(QStringLiteral("server.cert.pem")));
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QSslCertificate certificate(file.readAll());
    QCOMPARE(info.sha256Fingerprint,
             QString::fromLatin1(certificate.digest(QCryptographicHash::Sha256).toHex(':').toUpper()));
    QVERIFY(info.details.contains(QStringLiteral("Signature Algorithm")));
    QVERIFY(info.details.contains(QStringLiteral("Public-Key")));
#ifdef OUASIM_HAVE_ENCRYPTION
    QCOMPARE(info.version, QStringLiteral("V3"));
    QCOMPARE(info.signatureAlgorithm, QStringLiteral("sha256WithRSAEncryption"));
    QCOMPARE(info.signatureHashAlgorithm, QStringLiteral("SHA256"));
    QCOMPARE(info.publicKey, QStringLiteral("RSA (2048 bits)"));
    QCOMPARE(info.publicKeyParameters, QStringLiteral("05 00"));
    QCOMPARE(info.basicConstraints, QStringLiteral("CA:FALSE"));
    QVERIFY(info.extendedKeyUsage.isEmpty());
    QCOMPARE(info.subjectKeyIdentifier, QStringLiteral("AF:03:8B:C8:0D:EE:EF:7B:F0:63:E4:AF:8E:CF:D6:59:46:33:DC:C3"));
    QCOMPARE(info.authorityKeyIdentifier, info.subjectKeyIdentifier);
    QVERIFY(info.subjectAlternativeNames.contains(QStringLiteral("URI:urn:openuasim:server")));
    QVERIFY(info.subjectAlternativeNames.contains(QStringLiteral("DNS:localhost")));
    QVERIFY(info.keyUsage.contains(QStringLiteral("Digital Signature")));
    QVERIFY(info.keyUsage.contains(QStringLiteral("Key Encipherment")));
#endif
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
    QCOMPARE(QFileInfo(certificates.first().filePath).fileName(),
             QStringLiteral("OpenUaSim@%1.der").arg(QHostInfo::localHostName()));

    const PkiMaterial material = _store.material();
    QVERIFY(!material.certificate.isEmpty());
    QVERIFY(material.privateKey.contains("PRIVATE KEY"));
    const CertificateStore reopened(_directory->path());
    QVERIFY(reopened.hasServerCertificate());
    QCOMPARE(reopened.material().certificate, material.certificate);
    QCOMPARE(reopened.material().privateKey, material.privateKey);
}

///
/// \brief Loads a key pair copied from another host and updates its filename on installation.
///
void TestCertificateStore::loadsServerCertificateFromAnotherHost()
{
    const QString fileName = QStringLiteral("OpenUaSim@earlier-host.der");
    const QString own = _store.folder(Category::Application);
    const QString privateFolder = _store.root() + QStringLiteral("/own/private");
    QVERIFY(QDir().mkpath(own));
    QVERIFY(QDir().mkpath(privateFolder));
    const QString previous = own + QLatin1Char('/') + fileName;
    QVERIFY(QFile::copy(testData(QStringLiteral("stranger.der")), previous));
    QVERIFY(QFile::copy(testData(QStringLiteral("stranger.key.der")), privateFolder + QStringLiteral("/server.key")));
    QFile garbage(own + QStringLiteral("/OpenUaSim@000-invalid.der"));
    QVERIFY(garbage.open(QIODevice::WriteOnly));
    QCOMPARE(garbage.write("garbage"), qint64(7));
    garbage.close();
    CertificateStore reopened(_store.root());
    QVERIFY(reopened.hasServerCertificate());
    QCOMPARE(reopened.certificates().size(), 1);
    QCOMPARE(QFileInfo(reopened.certificates().first().filePath).fileName(), fileName);
    const PkiMaterial original = reopened.material();
    QVERIFY(!original.certificate.isEmpty());
    QVERIFY(!original.privateKey.isEmpty());
    QVERIFY(reopened.installServerCertificate(testData(QStringLiteral("server.cert.pem")),
                                               testData(QStringLiteral("server.key.pem"))));
    QVERIFY(!QFileInfo::exists(previous));
    QCOMPARE(reopened.certificates().size(), 1);
    QCOMPARE(QFileInfo(reopened.certificates().first().filePath).fileName(),
             QStringLiteral("OpenUaSim@%1.der").arg(QHostInfo::localHostName()));
    QVERIFY(reopened.material().certificate != original.certificate);
    const CertificateStore afterRestart(_store.root());
    QVERIFY(afterRestart.hasServerCertificate());
    QCOMPARE(afterRestart.material().certificate, reopened.material().certificate);
    QCOMPARE(afterRestart.material().privateKey, reopened.material().privateKey);
    QVERIFY(afterRestart.certificates().first().filePath.endsWith(QStringLiteral(".der")));
}

///
/// \brief Reports an unwritable certificate destination without creating a private key.
///
void TestCertificateStore::serverCertificateWriteFailure()
{
    const QString blocked = _store.folder(Category::Application)
                            + QStringLiteral("/OpenUaSim@%1.der").arg(QHostInfo::localHostName());
    QVERIFY(QDir().mkpath(blocked));
    QString error;
    QVERIFY(!_store.installServerCertificate(testData(QStringLiteral("server.cert.pem")),
                                             testData(QStringLiteral("server.key.pem")), &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!_store.hasServerCertificate());
    QVERIFY(_store.material().privateKey.isEmpty());
}

void TestCertificateStore::storesServerCertificateFromMemory()
{
    QVERIFY(!_store.hasServerCertificate());
    QFile certificate(testData(QStringLiteral("stranger.der")));
    QFile key(testData(QStringLiteral("stranger.key.der")));
    QVERIFY(certificate.open(QIODevice::ReadOnly) && key.open(QIODevice::ReadOnly));
    const QByteArray certificateData = certificate.readAll();
    const QByteArray keyData = key.readAll();

    QString error;
    QVERIFY(!_store.storeServerCertificate(QByteArray("garbage"), keyData, &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!_store.storeServerCertificate(certificateData, QByteArray(), &error));
    QVERIFY(!_store.hasServerCertificate());

    QVERIFY(_store.storeServerCertificate(certificateData, keyData));
    QVERIFY(_store.hasServerCertificate());
    QCOMPARE(_store.material().privateKey, keyData);
}

void TestCertificateStore::generatesSelfSignedCertificate()
{
    if (!CertificateGenerator::isAvailable()) {
        QString error;
        QVERIFY(CertificateGenerator::generate(QStringLiteral("urn:test"), {}, &error).certificate.isEmpty());
        QVERIFY(!error.isEmpty());
        QSKIP("Built without OUASIM_WITH_ENCRYPTION");
    }
    const GeneratedCertificate generated = CertificateGenerator::generate(
        QStringLiteral("urn:openuasim:test"), { QStringLiteral("sim-host"), QString(), QStringLiteral("sim-host") });
    QVERIFY(!generated.privateKey.isEmpty());
    const QSslCertificate certificate(generated.certificate, QSsl::Der);
    QVERIFY(!certificate.isNull());
    QVERIFY(certificate.isSelfSigned());
    QCOMPARE(certificate.subjectAlternativeNames().values(QSsl::DnsEntry).count(QStringLiteral("sim-host")), 1);
    QVERIFY(certificate.expiryDate() > QDateTime::currentDateTimeUtc().addYears(9));
    const QByteArray serial = QByteArray::fromHex(certificate.serialNumber());
    QCOMPARE(serial.size(), 20);
    QVERIFY((static_cast<unsigned char>(serial.front()) & 0x80) == 0);
    QVERIFY(serial != QByteArray(20, '\0'));
    const GeneratedCertificate second = CertificateGenerator::generate(QStringLiteral("urn:openuasim:test"), {});
    const QSslCertificate secondCertificate(second.certificate, QSsl::Der);
    QVERIFY(!secondCertificate.isNull());
    QVERIFY(secondCertificate.serialNumber() != certificate.serialNumber());
}

void TestCertificateStore::generatesWithLongSubjectNames()
{
    if (!CertificateGenerator::isAvailable())
        QSKIP("Built without OUASIM_WITH_ENCRYPTION");
    const QString applicationName = QCoreApplication::applicationName();
    const QString organizationName = QCoreApplication::organizationName();
    QCoreApplication::setApplicationName(QString(80, QLatin1Char('a')));
    QCoreApplication::setOrganizationName(QStringLiteral("Application Publisher"));

    const GeneratedCertificate generated = CertificateGenerator::generate(QStringLiteral("urn:openuasim:test"), {});
    QCoreApplication::setApplicationName(applicationName);
    QCoreApplication::setOrganizationName(organizationName);

    QVERIFY(!generated.privateKey.isEmpty());
    const QSslCertificate certificate(generated.certificate, QSsl::Der);
    QCOMPARE(certificate.subjectInfo(QSslCertificate::CommonName).value(0), QString(64, QLatin1Char('a')));
    QVERIFY(certificate.subjectInfo(QSslCertificate::Organization).isEmpty());
    QVERIFY(certificate.issuerInfo(QSslCertificate::Organization).isEmpty());
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
    ProjectConfig project = TestProject::create();
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
    ProjectConfig project = TestProject::create();
    EndpointInfo &endpoint = project.endpoints.first();
    endpoint.port = TestPort::freePort();
    endpoint.securityPolicy = QStringLiteral("Basic256Sha256");
    endpoint.securityMode = QStringLiteral("Sign & Encrypt");

    ServerController controller;
    controller.certificateStore().setRoot(_directory->path());
    QVERIFY(controller.installServerCertificate(testData(QStringLiteral("server.cert.pem")),
                                                testData(QStringLiteral("server.key.pem"))));
    const PkiMaterial installed = controller.certificateStore().material();
    controller.setProject(project);
    QVERIFY(controller.start());
    QCOMPARE(controller.certificateStore().material().certificate, installed.certificate);
#endif
}

void TestCertificateStore::startCreatesMissingServerCertificate()
{
#ifndef OUASIM_HAVE_ENCRYPTION
    QSKIP("Built without OUASIM_WITH_ENCRYPTION");
#else
    ProjectConfig project = TestProject::create();
    EndpointInfo &endpoint = project.endpoints.first();
    endpoint.port = TestPort::freePort();
    endpoint.securityPolicy = QStringLiteral("Aes256_Sha256_RsaPss");
    endpoint.securityMode = QStringLiteral("Sign & Encrypt");

    ServerController controller;
    controller.certificateStore().setRoot(_directory->path());
    QVERIFY(controller.importCertificate(testData(QStringLiteral("stranger.der")), Category::Trusted));
    controller.setProject(project);
    QSignalSpy changed(&controller, &ServerController::certificatesChanged);
    QSignalSpy warned(&controller, &ServerController::startWarning);
    QVERIFY(controller.start());
    QVERIFY(controller.certificateStore().hasServerCertificate());
    QCOMPARE(changed.count(), 1);
    QCOMPARE(warned.count(), 0);

    QFile certificate(testData(QStringLiteral("stranger.der")));
    QFile key(testData(QStringLiteral("stranger.key.der")));
    QVERIFY(certificate.open(QIODevice::ReadOnly) && key.open(QIODevice::ReadOnly));
    UaTestClient client;
    QCOMPARE(client.setEncryption(certificate.readAll(), key.readAll(), QStringLiteral("urn:openuasim:test:stranger")),
             UA_STATUSCODE_GOOD);
    QCOMPARE(client.connect(controller.server().endpointUrl()), UA_STATUSCODE_GOOD);
#endif
}

void TestCertificateStore::userCertificateLogsInByCommonName_data()
{
    QTest::addColumn<bool>("userCertificate");
    QTest::addColumn<bool>("trustedClientsOnly");
    QTest::addColumn<bool>("accepted");

    QTest::newRow("any certificate without trust list") << true << false << true;
    QTest::newRow("trusted certificate") << true << true << true;
    QTest::newRow("certificates not accepted") << false << false << false;
}

void TestCertificateStore::userCertificateLogsInByCommonName()
{
#ifndef OUASIM_HAVE_ENCRYPTION
    QSKIP("Built without OUASIM_WITH_ENCRYPTION");
#else
    QFETCH(bool, userCertificate);
    QFETCH(bool, trustedClientsOnly);
    QFETCH(bool, accepted);
    ProjectConfig project = TestProject::create();
    EndpointInfo &endpoint = project.endpoints.first();
    endpoint.port = TestPort::freePort();
    endpoint.securityPolicy = QStringLiteral("Aes256_Sha256_RsaPss");
    endpoint.securityMode = QStringLiteral("Sign & Encrypt");
    endpoint.allowAnonymous = false;
    endpoint.usernamePassword = true;
    endpoint.userCertificate = userCertificate;
    endpoint.trustedClientsOnly = trustedClientsOnly;
    project.users = { { QStringLiteral("operator"), QStringLiteral("secret") } };

    ServerController controller;
    controller.certificateStore().setRoot(_directory->path());
    QVERIFY(controller.importCertificate(testData(QStringLiteral("stranger.der")), Category::Trusted));
    controller.setProject(project);
    QVERIFY(controller.start());

    QFile certificate(testData(QStringLiteral("stranger.der")));
    QFile key(testData(QStringLiteral("stranger.key.der")));
    QVERIFY(certificate.open(QIODevice::ReadOnly) && key.open(QIODevice::ReadOnly));
    const QByteArray der = certificate.readAll();
    const QByteArray privateKey = key.readAll();
    UaTestClient client;
    QCOMPARE(client.setEncryption(der, privateKey, QStringLiteral("urn:openuasim:test:stranger")), UA_STATUSCODE_GOOD);
    const UA_StatusCode status = client.connectCertificate(controller.server().endpointUrl(), der, privateKey);

    QCOMPARE(status == UA_STATUSCODE_GOOD, accepted);
    if (!accepted)
        return;
    QTRY_COMPARE(controller.server().clientCount(), 1);
    const SessionInfo session = controller.server().sessions().first();
    QCOMPARE(session.user, QStringLiteral("Stranger Client"));
    QCOMPARE(session.userIdentity, QStringLiteral("Certificate"));
#endif
}

void TestCertificateStore::defaultProjectStartsWithEveryEndpoint()
{
#ifndef OUASIM_HAVE_ENCRYPTION
    QSKIP("Built without OUASIM_WITH_ENCRYPTION");
#else
    ProjectConfig project = DefaultProject::create();
    const int port = TestPort::freePort();
    for (EndpointInfo &endpoint : project.endpoints)
        endpoint.port = port;

    ServerController controller;
    controller.certificateStore().setRoot(_directory->path());
    controller.setProject(project);
    QSignalSpy warned(&controller, &ServerController::startWarning);
    QVERIFY(controller.start());
    QVERIFY(controller.certificateStore().hasServerCertificate());
    QCOMPARE(warned.count(), 0);

    UaTestClient anonymous;
    QCOMPARE(anonymous.connect(controller.server().endpointUrl()), UA_STATUSCODE_GOOD);

    QFile certificate(testData(QStringLiteral("stranger.der")));
    QFile key(testData(QStringLiteral("stranger.key.der")));
    QVERIFY(certificate.open(QIODevice::ReadOnly) && key.open(QIODevice::ReadOnly));
    UaTestClient secure;
    QCOMPARE(secure.setEncryption(certificate.readAll(), key.readAll(), QStringLiteral("urn:openuasim:test:stranger")),
             UA_STATUSCODE_GOOD);
    QCOMPARE(secure.connect(controller.server().endpointUrl()), UA_STATUSCODE_GOOD);
#endif
}

void TestCertificateStore::generateReplacesServerCertificate()
{
    ServerController controller;
    controller.certificateStore().setRoot(_directory->path());
    QSignalSpy changed(&controller, &ServerController::certificatesChanged);
    QString error;
    if (!CertificateGenerator::isAvailable()) {
        QVERIFY(!controller.generateServerCertificate(&error));
        QVERIFY(!error.isEmpty());
        QCOMPARE(changed.count(), 0);
        return;
    }
    QVERIFY(controller.installServerCertificate(testData(QStringLiteral("server.cert.pem")),
                                                testData(QStringLiteral("server.key.pem"))));
    const QByteArray installed = controller.certificateStore().material().certificate;
    QVERIFY(controller.generateServerCertificate(&error));
    QCOMPARE(changed.count(), 2);
    QVERIFY(controller.certificateStore().material().certificate != installed);
    QCOMPARE(controller.certificates().size(), 1);
    QCOMPARE(QFileInfo(controller.certificates().first().filePath).fileName(),
             QStringLiteral("OpenUaSim@%1.der").arg(QHostInfo::localHostName()));
}

///
/// \brief Covers startup independently of enabled secure endpoints.
///
void TestCertificateStore::ensuresCertificateBeforeServerStart_data()
{
    QTest::addColumn<bool>("emptyEndpoints");
    QTest::newRow("no-endpoints") << true;
    QTest::newRow("unsecured-endpoint") << false;
}

///
/// \brief Creates missing PKI material before the server runs and leaves it unchanged on repeated checks.
///
void TestCertificateStore::ensuresCertificateBeforeServerStart()
{
    QFETCH(bool, emptyEndpoints);
    ProjectConfig project = TestProject::create();
    project.server.applicationUri = QStringLiteral("urn:openuasim:startup-test");
    if (emptyEndpoints)
        project.endpoints.clear();
    else {
        project.endpoints.first().securityPolicy = QStringLiteral("None");
        project.endpoints.first().securityMode = QStringLiteral("None");
    }
    ServerController controller;
    controller.certificateStore().setRoot(_directory->path());
    controller.setProject(project);
    QSignalSpy changed(&controller, &ServerController::certificatesChanged);
    controller.ensureServerCertificate();
    QVERIFY(!controller.isRunning());
    if (!CertificateGenerator::isAvailable()) {
        QVERIFY(!controller.certificateStore().hasServerCertificate());
        QCOMPARE(changed.count(), 0);
        QSKIP("Built without OUASIM_WITH_ENCRYPTION");
    }
    QVERIFY(controller.certificateStore().hasServerCertificate());
    QCOMPARE(changed.count(), 1);
    QCOMPARE(controller.certificates().size(), 1);
    const CertificateInfo info = controller.certificates().first();
    QVERIFY(info.details.contains(project.server.applicationUri));
    QVERIFY(info.subjectAlternativeNames.contains(project.server.applicationUri));
    QVERIFY(info.extendedKeyUsage.contains(QStringLiteral("TLS Web Server Authentication")));
    QVERIFY(info.extendedKeyUsage.contains(QStringLiteral("TLS Web Client Authentication")));
    QVERIFY(!info.subjectKeyIdentifier.isEmpty());
    QVERIFY(info.authorityKeyIdentifier.isEmpty());
    QCOMPARE(QFileInfo(info.filePath).fileName(),
             QStringLiteral("OpenUaSim@%1.der").arg(QHostInfo::localHostName()));
    QCOMPARE(controller.log().last().category, LogEntry::Category::Security);
    QCOMPARE(controller.log().last().level, LogEntry::Level::Info);
    QVERIFY(!controller.isRestartRequired());

    const PkiMaterial material = controller.certificateStore().material();
    const qsizetype logSize = controller.log().size();
    controller.ensureServerCertificate();
    QCOMPARE(changed.count(), 1);
    QCOMPARE(controller.log().size(), logSize);
    QCOMPARE(controller.certificateStore().material().certificate, material.certificate);
    QCOMPARE(controller.certificateStore().material().privateKey, material.privateKey);
}

///
/// \brief Preserves an installed certificate and key during startup initialization.
///
void TestCertificateStore::ensureKeepsInstalledCertificate()
{
    ServerController controller;
    controller.certificateStore().setRoot(_directory->path());
    controller.setProject(TestProject::create());
    QVERIFY(controller.installServerCertificate(testData(QStringLiteral("server.cert.pem")),
                                                testData(QStringLiteral("server.key.pem"))));
    const PkiMaterial material = controller.certificateStore().material();
    const qsizetype logSize = controller.log().size();
    QSignalSpy changed(&controller, &ServerController::certificatesChanged);
    controller.ensureServerCertificate();
    QCOMPARE(changed.count(), 0);
    QCOMPARE(controller.log().size(), logSize);
    QCOMPARE(controller.certificateStore().material().certificate, material.certificate);
    QCOMPARE(controller.certificateStore().material().privateKey, material.privateKey);
    QVERIFY(!controller.isRunning());
}

///
/// \brief Records a certificate write failure without announcing a successful PKI change.
///
void TestCertificateStore::ensureReportsGenerationFailure()
{
    if (!CertificateGenerator::isAvailable())
        QSKIP("Built without OUASIM_WITH_ENCRYPTION");
    QFile blocked(_directory->filePath(QStringLiteral("own")));
    QVERIFY(blocked.open(QIODevice::WriteOnly));
    blocked.close();
    ServerController controller;
    controller.certificateStore().setRoot(_directory->path());
    controller.setProject(TestProject::create());
    QSignalSpy changed(&controller, &ServerController::certificatesChanged);
    controller.ensureServerCertificate();
    QVERIFY(!controller.certificateStore().hasServerCertificate());
    QCOMPARE(changed.count(), 0);
    QCOMPARE(controller.log().last().category, LogEntry::Category::Security);
    QCOMPARE(controller.log().last().level, LogEntry::Level::Error);
    QVERIFY(!controller.log().last().message.isEmpty());
    QVERIFY(!controller.isRunning());
}

void TestCertificateStore::addsCertificateFromMemory()
{
    QFile file(testData(QStringLiteral("client.der")));
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString stored = _store.addCertificate(file.readAll(), Category::Rejected);
    QVERIFY(QFileInfo::exists(stored));
    QCOMPARE(_store.certificates().first().category, Category::Rejected);

    QString error;
    QVERIFY(_store.addCertificate(QByteArray("garbage"), Category::Trusted, &error).isEmpty());
    QVERIFY(!error.isEmpty());
}

void TestCertificateStore::findsStoredCertificateByContent()
{
    QFile client(testData(QStringLiteral("client.der")));
    QFile stranger(testData(QStringLiteral("stranger.der")));
    QVERIFY(client.open(QIODevice::ReadOnly) && stranger.open(QIODevice::ReadOnly));
    const QByteArray clientDer = client.readAll();
    QVERIFY(!_store.isStored(clientDer));

    const QString stored = _store.addCertificate(clientDer, Category::Issuer);
    const QString renamed = QFileInfo(stored).absolutePath() + QStringLiteral("/renamed.der");
    QVERIFY(QFile::rename(stored, renamed));
    QVERIFY(_store.isStored(clientDer));
    QVERIFY(!_store.isStored(stranger.readAll()));
    QVERIFY(!_store.isStored(QByteArray("garbage")));
}

void TestCertificateStore::rejectedClientLandsInRejectedFolder()
{
#ifndef OUASIM_HAVE_ENCRYPTION
    QSKIP("Built without OUASIM_WITH_ENCRYPTION");
#else
    ProjectConfig project = TestProject::create();
    EndpointInfo &endpoint = project.endpoints.first();
    endpoint.port = TestPort::freePort();
    endpoint.securityPolicy = QStringLiteral("Basic256Sha256");
    endpoint.securityMode = QStringLiteral("Sign & Encrypt");
    endpoint.trustedClientsOnly = true;

    ServerController controller;
    controller.certificateStore().setRoot(_directory->path());
    QVERIFY(controller.installServerCertificate(testData(QStringLiteral("server.cert.pem")),
                                                testData(QStringLiteral("server.key.pem"))));
    QVERIFY(controller.importCertificate(testData(QStringLiteral("client.der")), Category::Trusted));
    controller.setProject(project);
    QVERIFY(controller.start());
    QSignalSpy changed(&controller, &ServerController::certificatesChanged);

    QFile certificate(testData(QStringLiteral("stranger.der")));
    QFile key(testData(QStringLiteral("stranger.key.der")));
    QVERIFY(certificate.open(QIODevice::ReadOnly) && key.open(QIODevice::ReadOnly));
    UaTestClient client;
    QCOMPARE(client.setEncryption(certificate.readAll(), key.readAll(), QStringLiteral("urn:openuasim:test:stranger")),
             UA_STATUSCODE_GOOD);
    QVERIFY(client.connect(controller.server().endpointUrl()) != UA_STATUSCODE_GOOD);

    QTRY_VERIFY(changed.count() >= 1);
    QList<CertificateInfo> rejected;
    for (const CertificateInfo &info : controller.certificates()) {
        if (info.category == Category::Rejected)
            rejected << info;
    }
    QCOMPARE(rejected.size(), 1);
    QCOMPARE(rejected.first().commonName, QStringLiteral("Stranger Client"));
    QVERIFY(!controller.isRestartRequired());
#endif
}

namespace {

///
/// \brief Starts a server whose only endpoint is Basic256Sha256 / Sign & Encrypt, then connects as the stranger client.
/// \param controller Controller to start; its store must already point at the test directory.
/// \param trustedClientsOnly Whether the endpoint accepts only trusted clients.
/// \return Connection status.
///
UA_StatusCode connectStranger(ServerController &controller, bool trustedClientsOnly)
{
    ProjectConfig project = TestProject::create();
    EndpointInfo &endpoint = project.endpoints.first();
    endpoint.port = TestPort::freePort();
    endpoint.securityPolicy = QStringLiteral("Basic256Sha256");
    endpoint.securityMode = QStringLiteral("Sign & Encrypt");
    endpoint.trustedClientsOnly = trustedClientsOnly;
    controller.setProject(project);
    if (!controller.start())
        return UA_STATUSCODE_BADINTERNALERROR;

    QFile certificate(testData(QStringLiteral("stranger.der")));
    QFile key(testData(QStringLiteral("stranger.key.der")));
    if (!certificate.open(QIODevice::ReadOnly) || !key.open(QIODevice::ReadOnly))
        return UA_STATUSCODE_BADINTERNALERROR;
    UaTestClient client;
    const UA_StatusCode encryption =
        client.setEncryption(certificate.readAll(), key.readAll(), QStringLiteral("urn:openuasim:test:stranger"));
    return encryption != UA_STATUSCODE_GOOD ? encryption : client.connect(controller.server().endpointUrl());
}

}

void TestCertificateStore::trustedClientsOnlyRejectsClientsWithEmptyTrustList()
{
#ifndef OUASIM_HAVE_ENCRYPTION
    QSKIP("Built without OUASIM_WITH_ENCRYPTION");
#else
    ServerController controller;
    controller.certificateStore().setRoot(_directory->path());
    QSignalSpy changed(&controller, &ServerController::certificatesChanged);
    QVERIFY(connectStranger(controller, true) != UA_STATUSCODE_GOOD);

    QTRY_VERIFY(changed.count() >= 2);
    QStringList rejected;
    for (const CertificateInfo &info : controller.certificates()) {
        if (info.category == Category::Rejected)
            rejected << info.commonName;
    }
    QCOMPARE(rejected, QStringList { QStringLiteral("Stranger Client") });
#endif
}

void TestCertificateStore::anyClientConnectsWithoutTrustedClientsOnly()
{
#ifndef OUASIM_HAVE_ENCRYPTION
    QSKIP("Built without OUASIM_WITH_ENCRYPTION");
#else
    ServerController controller;
    controller.certificateStore().setRoot(_directory->path());
    QVERIFY(controller.importCertificate(testData(QStringLiteral("client.der")), Category::Trusted));
    QCOMPARE(connectStranger(controller, false), UA_STATUSCODE_GOOD);
#endif
}

void TestCertificateStore::unverifiedClientLandsInRejectedFolder()
{
#ifndef OUASIM_HAVE_ENCRYPTION
    QSKIP("Built without OUASIM_WITH_ENCRYPTION");
#else
    ServerController controller;
    controller.certificateStore().setRoot(_directory->path());
    QSignalSpy changed(&controller, &ServerController::certificatesChanged);
    QCOMPARE(connectStranger(controller, false), UA_STATUSCODE_GOOD);

    QTRY_VERIFY(!controller.sessions().isEmpty());
    QStringList rejected;
    for (const CertificateInfo &info : controller.certificates()) {
        if (info.category == Category::Rejected)
            rejected << info.commonName;
    }
    QCOMPARE(rejected, QStringList { QStringLiteral("Stranger Client") });
    QVERIFY(changed.count() >= 1);
    QVERIFY(std::any_of(controller.log().cbegin(), controller.log().cend(), [](const LogEntry &entry) {
        return entry.category == LogEntry::Category::Security && entry.message.contains(QStringLiteral("Stranger"));
    }));
    QVERIFY(!controller.isRestartRequired());
#endif
}

void TestCertificateStore::unverifiedClientAlreadyStoredIsNotCopied()
{
#ifndef OUASIM_HAVE_ENCRYPTION
    QSKIP("Built without OUASIM_WITH_ENCRYPTION");
#else
    ServerController controller;
    controller.certificateStore().setRoot(_directory->path());
    QVERIFY(controller.importCertificate(testData(QStringLiteral("stranger.der")), Category::Trusted));
    QCOMPARE(connectStranger(controller, false), UA_STATUSCODE_GOOD);

    QTRY_VERIFY(!controller.sessions().isEmpty());
    QList<Category> categories;
    for (const CertificateInfo &info : controller.certificates()) {
        if (info.commonName == QStringLiteral("Stranger Client"))
            categories << info.category;
    }
    QCOMPARE(categories, QList<Category> { Category::Trusted });
#endif
}

QTEST_GUILESS_MAIN(TestCertificateStore)

#include "test_certificatestore.moc"
