// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QSignalSpy>
#include <QTest>

#include <QUaServer>

#include "server/simserver.h"
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
    QCOMPARE(server.sessions().first().userIdentity, QStringLiteral("UserName"));

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
    QCOMPARE(client.connect(server.endpointUrl()), UA_STATUSCODE_GOOD);
    QTRY_COMPARE(server.clientCount(), 1);

    const SessionInfo session = server.sessions().first();
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
    QVERIFY(server.sessions().first().lastActivity.isValid());
    QCOMPARE(sessions.count(), 1);

    server.refreshSessionDiagnostics();
    QCOMPARE(sessions.count(), 1);
}

QTEST_GUILESS_MAIN(TestSimServer)

#include "test_simserver.moc"
