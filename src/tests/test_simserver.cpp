// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QSignalSpy>
#include <QTest>

#include <QUaServer>

#include "pki/certificategenerator.h"
#include "server/simserver.h"
#include "testdiscoveryserver.h"
#include "testport.h"
#include "uatestclient.h"

namespace {

ProjectConfig project(quint16 port)
{
    EndpointInfo endpoint;
    endpoint.protocol = QStringLiteral("opc.tcp");
    endpoint.hostname = QStringLiteral("localhost");
    endpoint.port = port;
    endpoint.securityPolicy = QStringLiteral("None");
    endpoint.securityMode = QStringLiteral("None");
    ProjectConfig config;
    config.endpoints.append(endpoint);
    return config;
}

bool containsMessage(const QSignalSpy &spy, const QString &text)
{
    for (const QList<QVariant> &arguments : spy) {
        if (arguments.first().value<LogEntry>().message.contains(text))
            return true;
    }
    return false;
}

int countMessages(const QSignalSpy &spy, const QString &text)
{
    int count = 0;
    for (const QList<QVariant> &arguments : spy) {
        if (arguments.first().value<LogEntry>().message.contains(text))
            ++count;
    }
    return count;
}

LogEntry::Level levelOf(const QSignalSpy &spy, const QString &text)
{
    for (const QList<QVariant> &arguments : spy) {
        const LogEntry entry = arguments.first().value<LogEntry>();
        if (entry.message.contains(text))
            return entry.level;
    }
    return LogEntry::Level::Error;
}

}

Q_DECLARE_METATYPE(LogEntry)

class TestSimServer : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void startsAndStops();
    void resetReplacesServer();
    void restartsAfterStop();
    void reportsInvalidEndpoints();
    void reportsPortInUse();
    void reportsMismatchedSecurity();
    void refusesSecureOnlyEndpointsWithoutCertificate();
    void warnsWhenSecureEndpointsAreDropped();
    void tracksAnonymousSession();
    void stopClosesSessions();
    void acceptsConfiguredUser();
    void rejectsAnonymousWhenDisabled();
    void refusesUsernameEndpointsWithoutAccounts();
    void sessionDescribesSecurityAndEndpoint();
    void diagnosticsCountSubscriptions();
    void registersAtDiscoveryServer();
    void logsRegistrationOnlyWhenItChanges();
    void warnsWhenDiscoveryServerIsUnreachable();
    void explainsCertificateRejectedByDiscoveryServer();
    void checksDiscoveryServerCertificateOnlyWhenAsked_data();
    void checksDiscoveryServerCertificateOnlyWhenAsked();
    void disabledDiscoveryRegistersNowhere();

private:
    quint16 _port = 0;
};

void TestSimServer::init()
{
    _port = TestPort::freePort();
    QVERIFY(_port != 0);
}

void TestSimServer::startsAndStops()
{
    SimServer server;
    QSignalSpy running(&server, &SimServer::runningChanged);
    QSignalSpy log(&server, &SimServer::logAppended);

    QVERIFY(server.start(project(_port)));
    QVERIFY(server.isRunning());
    QVERIFY(server.startTime().isValid());
    QCOMPARE(server.endpointUrl(), QStringLiteral("opc.tcp://localhost:%1").arg(_port));
    QVERIFY(server.start(project(_port)));

    server.stop();
    QVERIFY(!server.isRunning());
    server.stop();

    QCOMPARE(running.count(), 2);
    QCOMPARE(running.at(0).first().toBool(), true);
    QCOMPARE(running.at(1).first().toBool(), false);
    QVERIFY(containsMessage(log, QStringLiteral("Server started on")));
    QVERIFY(containsMessage(log, QStringLiteral("Server stopped")));
}

void TestSimServer::resetReplacesServer()
{
    SimServer server;
    QVERIFY(server.uaServer()->loadNodeSet(QStringLiteral(OUASIM_TEST_DATA_DIR "/mixer.NodeSet2.xml")).isOk());
    QVERIFY(server.start(project(_port)));
    QSignalSpy running(&server, &SimServer::runningChanged);

    server.reset();
    QVERIFY(!server.isRunning());
    QCOMPARE(running.count(), 1);
    QVERIFY(!server.uaServer()->namespaces().contains(QStringLiteral("urn:openuasim:test:mixer")));

    QVERIFY(server.start(project(_port)));
    UaTestClient client;
    QCOMPARE(client.connect(server.endpointUrl()), UA_STATUSCODE_GOOD);
    QTRY_COMPARE(server.clientCount(), 1);
}

void TestSimServer::restartsAfterStop()
{
    SimServer server;
    QVERIFY(server.start(project(_port)));
    server.stop();

    const quint16 otherPort = TestPort::freePort();
    QVERIFY(server.start(project(otherPort)));
    QCOMPARE(server.endpointUrl(), QStringLiteral("opc.tcp://localhost:%1").arg(otherPort));

    UaTestClient client;
    QCOMPARE(client.connect(server.endpointUrl()), UA_STATUSCODE_GOOD);
}

void TestSimServer::reportsInvalidEndpoints()
{
    SimServer server;
    QSignalSpy failed(&server, &SimServer::startFailed);
    QVERIFY(!server.start(ProjectConfig()));
    QVERIFY(!server.isRunning());
    QCOMPARE(failed.count(), 1);
    QCOMPARE(failed.first().first().toString(), QStringLiteral("No endpoint is enabled"));
}

void TestSimServer::reportsPortInUse()
{
    SimServer blocker;
    QVERIFY(blocker.start(project(_port)));

    SimServer server;
    QSignalSpy failed(&server, &SimServer::startFailed);
    QVERIFY(!server.start(project(_port)));
    QVERIFY(!server.isRunning());
    QCOMPARE(failed.count(), 1);
    QVERIFY(failed.first().first().toString().contains(QString::number(_port)));
}

void TestSimServer::reportsMismatchedSecurity()
{
    ProjectConfig config = project(_port);
    config.endpoints.first().securityPolicy = QStringLiteral("Aes256_Sha256_RsaPss");

    SimServer server;
    QSignalSpy failed(&server, &SimServer::startFailed);
    QVERIFY(!server.start(config));
    QCOMPARE(failed.count(), 1);
    QVERIFY(failed.first().first().toString().contains(QStringLiteral("with mode 'None'")));
}

void TestSimServer::refusesSecureOnlyEndpointsWithoutCertificate()
{
    ProjectConfig config = project(_port);
    config.endpoints.first().securityPolicy = QStringLiteral("Basic256Sha256");
    config.endpoints.first().securityMode = QStringLiteral("Sign & Encrypt");

    SimServer server;
    QSignalSpy failed(&server, &SimServer::startFailed);
    QVERIFY(!server.start(config));
    QCOMPARE(failed.count(), 1);
    QVERIFY(failed.first().first().toString().startsWith(QStringLiteral("Secure endpoints need")));
}

void TestSimServer::warnsWhenSecureEndpointsAreDropped()
{
    ProjectConfig config = project(_port);
    EndpointInfo secure = config.endpoints.first();
    secure.securityPolicy = QStringLiteral("Aes256_Sha256_RsaPss");
    secure.securityMode = QStringLiteral("Sign & Encrypt");
    config.endpoints.append(secure);

    SimServer server;
    QSignalSpy warned(&server, &SimServer::startWarning);
    QVERIFY(server.start(config));
    QCOMPARE(warned.count(), 1);
    QVERIFY(warned.first().first().toString().endsWith(QStringLiteral("only unsecured endpoints are offered")));
}

void TestSimServer::tracksAnonymousSession()
{
    SimServer server;
    QVERIFY(server.start(project(_port)));
    QSignalSpy sessions(&server, &SimServer::sessionsChanged);

    UaTestClient client;
    QCOMPARE(client.connect(server.endpointUrl()), UA_STATUSCODE_GOOD);
    QTRY_COMPARE(server.clientCount(), 1);
    const SessionInfo opened = server.sessions().first();
    QVERIFY(opened.active);
    QCOMPARE(opened.user, QStringLiteral("Anonymous"));
    QCOMPARE(opened.endpointUrl, server.endpointUrl());

    QCOMPARE(client.disconnect(), UA_STATUSCODE_GOOD);
    QTRY_COMPARE(server.clientCount(), 0);
    QCOMPARE(server.sessions().size(), 1);
    QVERIFY(!server.sessions().first().active);
    QVERIFY(sessions.count() >= 2);
}

void TestSimServer::stopClosesSessions()
{
    SimServer server;
    QVERIFY(server.start(project(_port)));
    UaTestClient client;
    QCOMPARE(client.connect(server.endpointUrl()), UA_STATUSCODE_GOOD);
    QTRY_COMPARE(server.clientCount(), 1);

    server.stop();
    QCOMPARE(server.clientCount(), 0);
    QCOMPARE(server.sessions().size(), 1);
    QVERIFY(!server.sessions().first().active);
}

void TestSimServer::acceptsConfiguredUser()
{
    ProjectConfig config = project(_port);
    EndpointInfo &endpoint = config.endpoints.first();
    endpoint.allowAnonymous = false;
    endpoint.usernamePassword = true;
    config.users = { { QStringLiteral("operator"), QStringLiteral("secret") },
                     { QStringLiteral("engineer"), QStringLiteral("other") } };

    SimServer server;
    QVERIFY(server.start(config));

    UaTestClient wrongPassword;
    QVERIFY(wrongPassword.connectUsername(server.endpointUrl(), QStringLiteral("operator"),
                                          QStringLiteral("wrong")) != UA_STATUSCODE_GOOD);

    UaTestClient client;
    QCOMPARE(client.connectUsername(server.endpointUrl(), QStringLiteral("operator"), QStringLiteral("secret")),
             UA_STATUSCODE_GOOD);
    QTRY_COMPARE(server.clientCount(), 1);
    QCOMPARE(server.sessions().first().user, QStringLiteral("operator"));
    QCOMPARE(server.sessions().first().userIdentity, QStringLiteral("Username"));

    UaTestClient engineer;
    QCOMPARE(engineer.connectUsername(server.endpointUrl(), QStringLiteral("engineer"), QStringLiteral("other")),
             UA_STATUSCODE_GOOD);
}

void TestSimServer::refusesUsernameEndpointsWithoutAccounts()
{
    ProjectConfig config = project(_port);
    config.endpoints.first().usernamePassword = true;
    SimServer server;
    QSignalSpy failed(&server, &SimServer::startFailed);
    QVERIFY(!server.start(config));
    QCOMPARE(failed.count(), 1);
    QVERIFY(failed.first().first().toString().contains(QStringLiteral("no user accounts")));
}

void TestSimServer::rejectsAnonymousWhenDisabled()
{
    ProjectConfig config = project(_port);
    EndpointInfo &endpoint = config.endpoints.first();
    endpoint.allowAnonymous = false;
    endpoint.usernamePassword = true;
    config.users = { { QStringLiteral("operator"), QString() } };

    SimServer server;
    QVERIFY(server.start(config));
    UaTestClient client;
    QVERIFY(client.connect(server.endpointUrl()) != UA_STATUSCODE_GOOD);
}

void TestSimServer::sessionDescribesSecurityAndEndpoint()
{
    SimServer server;
    QVERIFY(server.start(project(_port)));
    UaTestClient client;
    client.setSessionName(QStringLiteral("Explorer@HOST/1234-abcd"));
    QCOMPARE(client.connect(server.endpointUrl()), UA_STATUSCODE_GOOD);
    QTRY_COMPARE(server.clientCount(), 1);

    const SessionInfo session = server.sessions().first();
    QCOMPARE(session.name, QStringLiteral("Explorer@HOST/1234-abcd"));
    QCOMPARE(session.securityMode, QStringLiteral("None"));
    QCOMPARE(session.userIdentity, QStringLiteral("Anonymous"));
    QVERIFY(session.endpointUrl.startsWith(QStringLiteral("opc.tcp://")));
}

void TestSimServer::diagnosticsCountSubscriptions()
{
    SimServer server;
    QVERIFY(server.start(project(_port)));
    QSignalSpy sessions(&server, &SimServer::sessionsChanged);
    UaTestClient client;
    QCOMPARE(client.connect(server.endpointUrl()), UA_STATUSCODE_GOOD);
    QTRY_COMPARE(server.clientCount(), 1);
    QCOMPARE(server.sessions().first().subscriptions, 0);

    QCOMPARE(client.monitorValue(QStringLiteral("i=2258")), UA_STATUSCODE_GOOD);
    sessions.clear();
    server.refreshSessionDiagnostics();
    QCOMPARE(server.sessions().first().subscriptions, 1);
    const SubscriptionInfo subscription = server.sessions().first().subscriptionList.first();
    QVERIFY(subscription.id > 0);
    QCOMPARE(subscription.monitoredItems, 1u);
    QVERIFY(subscription.publishingEnabled);
    QVERIFY(subscription.publishingIntervalMs > 0.0);
    QVERIFY(server.sessions().first().lastActivity.isValid());
    QCOMPARE(sessions.count(), 1);

    server.refreshSessionDiagnostics();
    QCOMPARE(sessions.count(), 1);
}

void TestSimServer::registersAtDiscoveryServer()
{
    TestDiscoveryServer lds;
    QVERIFY(lds.start());
    ProjectConfig config = project(_port);
    config.server.discovery = { true, lds.url(), 600 };
    SimServer server;
    QSignalSpy log(&server, &SimServer::logAppended);

    QVERIFY(server.start(config));
    const QString registered = QStringLiteral("Registered at the discovery server %1").arg(lds.url());
    QTRY_VERIFY(containsMessage(log, registered));
    QCOMPARE(levelOf(log, registered), LogEntry::Level::Info);
    QCOMPARE(lds.registeredServers(), QStringList { config.server.applicationUri });

    server.stop();
    QVERIFY(containsMessage(log, QStringLiteral("Unregistered from the discovery server %1").arg(lds.url())));
    QVERIFY(lds.registeredServers().isEmpty());
}

void TestSimServer::logsRegistrationOnlyWhenItChanges()
{
    TestDiscoveryServer lds;
    QVERIFY(lds.start());
    ProjectConfig config = project(_port);
    config.server.discovery = { true, lds.url(), 1 };
    SimServer server;
    QSignalSpy log(&server, &SimServer::logAppended);
    QSignalSpy renewals(server.uaServer(), &QUaServer::discoveryRegistrationFinished);

    QVERIFY(server.start(config));
    QTRY_VERIFY_WITH_TIMEOUT(renewals.count() >= 3, 10000);
    QCOMPARE(countMessages(log, QStringLiteral("Registered at the discovery server")), 1);
}

void TestSimServer::warnsWhenDiscoveryServerIsUnreachable()
{
    const QString url = QStringLiteral("opc.tcp://127.0.0.1:%1").arg(TestPort::freePort());
    ProjectConfig config = project(_port);
    config.server.discovery = { true, url, 600 };
    SimServer server;
    QSignalSpy log(&server, &SimServer::logAppended);

    QVERIFY(server.start(config));
    const QString failed = QStringLiteral("Could not register at the discovery server %1").arg(url);
    QTRY_VERIFY_WITH_TIMEOUT(containsMessage(log, failed), 10000);
    QCOMPARE(levelOf(log, failed), LogEntry::Level::Warning);
    QVERIFY(server.isRunning());

    server.stop();
    QVERIFY(!containsMessage(log, QStringLiteral("Unregistered from the discovery server")));
    QVERIFY(!containsMessage(log, QStringLiteral("Could not unregister from the discovery server")));
}

void TestSimServer::explainsCertificateRejectedByDiscoveryServer()
{
    if (!CertificateGenerator::isAvailable())
        QSKIP("Built without OUASIM_WITH_ENCRYPTION");
    // the URI open62541 announces by default, so the discovery server certificate matches its description
    const GeneratedCertificate ldsCertificate =
        CertificateGenerator::generate(QStringLiteral("urn:open62541.unconfigured.application"), {});
    const GeneratedCertificate unrelated = CertificateGenerator::generate(QStringLiteral("urn:openuasim:other"), {});
    TestDiscoveryServer lds;
    QVERIFY(lds.start(ldsCertificate.certificate, ldsCertificate.privateKey, { unrelated.certificate }));
    ProjectConfig config = project(_port);
    config.server.discovery = { true, lds.url(), 600 };
    const GeneratedCertificate own = CertificateGenerator::generate(config.server.applicationUri, {});
    SimServer server;
    server.setPkiMaterial({ own.certificate, own.privateKey, {}, {} });
    QSignalSpy log(&server, &SimServer::logAppended);

    QVERIFY(server.start(config));
    const QString rejected = QStringLiteral("The discovery server %1 rejected the server certificate").arg(lds.url());
    QTRY_VERIFY_WITH_TIMEOUT(containsMessage(log, rejected), 10000);
    QCOMPARE(levelOf(log, rejected), LogEntry::Level::Warning);
    QVERIFY(lds.registeredServers().isEmpty());
}

void TestSimServer::checksDiscoveryServerCertificateOnlyWhenAsked_data()
{
    QTest::addColumn<bool>("verify");
    QTest::addColumn<bool>("trustedClientsOnly");
    QTest::addColumn<bool>("ldsTrusted");
    QTest::addColumn<bool>("registered");

    QTest::newRow("unchecked while clients must be trusted") << false << true << false << true;
    QTest::newRow("checked, untrusted") << true << false << false << false;
    QTest::newRow("checked, trusted") << true << false << true << true;
}

void TestSimServer::checksDiscoveryServerCertificateOnlyWhenAsked()
{
    if (!CertificateGenerator::isAvailable())
        QSKIP("Built without OUASIM_WITH_ENCRYPTION");
    QFETCH(bool, verify);
    QFETCH(bool, trustedClientsOnly);
    QFETCH(bool, ldsTrusted);
    QFETCH(bool, registered);

    const GeneratedCertificate ldsCertificate =
        CertificateGenerator::generate(QStringLiteral("urn:open62541.unconfigured.application"), {});
    TestDiscoveryServer lds;
    QVERIFY(lds.start(ldsCertificate.certificate, ldsCertificate.privateKey));
    ProjectConfig config = project(_port);
    config.server.discovery = { true, lds.url(), 600, verify };
    EndpointInfo &endpoint = config.endpoints.first();
    endpoint.securityPolicy = QStringLiteral("Basic256Sha256");
    endpoint.securityMode = QStringLiteral("Sign & Encrypt");
    endpoint.trustedClientsOnly = trustedClientsOnly;
    const GeneratedCertificate own = CertificateGenerator::generate(config.server.applicationUri, {});
    const QList<QByteArray> trusted = ldsTrusted ? QList<QByteArray> { ldsCertificate.certificate } : QList<QByteArray>();
    SimServer server;
    server.setPkiMaterial({ own.certificate, own.privateKey, trusted, {} });
    QSignalSpy log(&server, &SimServer::logAppended);
    QSignalSpy rejected(&server, &SimServer::discoveryServerCertificateRejected);

    QVERIFY(server.start(config));
    const QString success = QStringLiteral("Registered at the discovery server %1").arg(lds.url());
    const QString untrusted =
        QStringLiteral("The server does not trust the certificate of the discovery server %1").arg(lds.url());
    QTRY_VERIFY_WITH_TIMEOUT(containsMessage(log, success) || containsMessage(log, untrusted), 10000);
    QCOMPARE(containsMessage(log, success), registered);
    if (registered) {
        QCOMPARE(rejected.count(), 0);
        return;
    }
    QCOMPARE(levelOf(log, untrusted), LogEntry::Level::Warning);
    QCOMPARE(rejected.count(), 1);
    QCOMPARE(rejected.first().at(0).toByteArray(), ldsCertificate.certificate);
    QCOMPARE(rejected.first().at(1).toString(), QStringLiteral("BadCertificateUntrusted"));
    QVERIFY(!containsMessage(log, QStringLiteral("Could not register at the discovery server")));
}

void TestSimServer::disabledDiscoveryRegistersNowhere()
{
    ProjectConfig config = project(_port);
    config.server.discovery = { false, QStringLiteral("opc.tcp://localhost:4840"), 600 };
    SimServer server;
    QVERIFY(server.start(config));
    QCOMPARE(server.uaServer()->discoveryServerUrl(), QString());
}

QTEST_GUILESS_MAIN(TestSimServer)

#include "test_simserver.moc"
