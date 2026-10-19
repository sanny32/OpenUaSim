// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QTest>

#include "server/listenersettings.h"

namespace {

EndpointInfo endpoint(const QString &name, int port = kDefaultEndpointPort, const QString &policy = QStringLiteral("None"),
                      const QString &mode = QStringLiteral("None"))
{
    EndpointInfo info;
    info.protocol = QStringLiteral("opc.tcp");
    info.hostname = QStringLiteral("host-") + name;
    info.port = port;
    info.securityPolicy = policy;
    info.securityMode = mode;
    return info;
}

}

class TestListenerSettings : public QObject
{
    Q_OBJECT

private slots:
    void mergesEnabledEndpoints();
    void skipsDisabledEndpoints();
    void usernameWhenAnyEnabledEndpointAcceptsIt();
    void anonymousWhenAnyEndpointAllowsIt();
    void trustedClientsOnlyWhenAnyEnabledEndpointIs();
    void rejectsNoEnabledEndpoint();
    void rejectsDifferentPorts();
    void rejectsInvalidPort();
    void rejectsUnknownPolicy();
    void rejectsUnknownMode();
    void securityPairs_data();
    void securityPairs();
    void rejectsMismatchedPolicyAndMode();
    void usesSecurityOfEnabledEndpoints();
};

void TestListenerSettings::mergesEnabledEndpoints()
{
    const ListenerSettings settings = ListenerSettingsBuilder::build({
        endpoint(QStringLiteral("a"), 4841),
        endpoint(QStringLiteral("b"), 4841, QStringLiteral("Basic256Sha256"), QStringLiteral("Sign & Encrypt")),
        endpoint(QStringLiteral("c"), 4841, QStringLiteral("Basic256Sha256"), QStringLiteral("Sign")),
    });
    QVERIFY2(settings.ok(), qPrintable(settings.error));
    QCOMPARE(settings.hostname, QStringLiteral("host-a"));
    QCOMPARE(settings.port, quint16(4841));
    QCOMPARE(settings.securityPolicies, QStringList({ QStringLiteral("None"), QStringLiteral("Basic256Sha256") }));
    QCOMPARE(settings.securityModes,
             QStringList({ QStringLiteral("None"), QStringLiteral("Sign & Encrypt"), QStringLiteral("Sign") }));
    QVERIFY(settings.anonymousAllowed);
}

void TestListenerSettings::skipsDisabledEndpoints()
{
    EndpointInfo disabled = endpoint(QStringLiteral("off"), 9999, QStringLiteral("Unknown"));
    disabled.enabled = false;
    const ListenerSettings settings = ListenerSettingsBuilder::build({ disabled, endpoint(QStringLiteral("on")) });
    QVERIFY2(settings.ok(), qPrintable(settings.error));
    QCOMPARE(settings.hostname, QStringLiteral("host-on"));
    QCOMPARE(settings.port, quint16(kDefaultEndpointPort));
}

void TestListenerSettings::usernameWhenAnyEnabledEndpointAcceptsIt()
{
    EndpointInfo secured = endpoint(QStringLiteral("a"));
    secured.usernamePassword = true;
    QVERIFY(!ListenerSettingsBuilder::build({ endpoint(QStringLiteral("a")) }).usernameAllowed);
    QVERIFY(ListenerSettingsBuilder::build({ endpoint(QStringLiteral("b")), secured }).usernameAllowed);
    secured.enabled = false;
    QVERIFY(!ListenerSettingsBuilder::build({ endpoint(QStringLiteral("b")), secured }).usernameAllowed);
}

void TestListenerSettings::anonymousWhenAnyEndpointAllowsIt()
{
    EndpointInfo closed = endpoint(QStringLiteral("a"));
    closed.allowAnonymous = false;
    QVERIFY(!ListenerSettingsBuilder::build({ closed }).anonymousAllowed);
    QVERIFY(ListenerSettingsBuilder::build({ closed, endpoint(QStringLiteral("b")) }).anonymousAllowed);
}

void TestListenerSettings::trustedClientsOnlyWhenAnyEnabledEndpointIs()
{
    EndpointInfo strict = endpoint(QStringLiteral("b"));
    strict.trustedClientsOnly = true;
    QVERIFY(!ListenerSettingsBuilder::build({ endpoint(QStringLiteral("a")) }).trustedClientsOnly);
    QVERIFY(ListenerSettingsBuilder::build({ endpoint(QStringLiteral("a")), strict }).trustedClientsOnly);
    strict.enabled = false;
    QVERIFY(!ListenerSettingsBuilder::build({ endpoint(QStringLiteral("a")), strict }).trustedClientsOnly);
}

void TestListenerSettings::rejectsNoEnabledEndpoint()
{
    QCOMPARE(ListenerSettingsBuilder::build({}).error, QStringLiteral("No endpoint is enabled"));
    EndpointInfo disabled = endpoint(QStringLiteral("a"));
    disabled.enabled = false;
    QVERIFY(!ListenerSettingsBuilder::build({ disabled }).ok());
}

void TestListenerSettings::rejectsDifferentPorts()
{
    const ListenerSettings settings =
        ListenerSettingsBuilder::build({ endpoint(QStringLiteral("a"), 4840), endpoint(QStringLiteral("b"), 4841) });
    QVERIFY(settings.error.contains(QStringLiteral("must share one port")));
}

void TestListenerSettings::rejectsInvalidPort()
{
    QVERIFY(!ListenerSettingsBuilder::build({ endpoint(QStringLiteral("a"), 0) }).ok());
    QVERIFY(!ListenerSettingsBuilder::build({ endpoint(QStringLiteral("a"), 70000) }).ok());
}

void TestListenerSettings::rejectsUnknownPolicy()
{
    const ListenerSettings settings =
        ListenerSettingsBuilder::build({ endpoint(QStringLiteral("a"), 4840, QStringLiteral("Rot13")) });
    QVERIFY(settings.error.contains(QStringLiteral("unknown security policy 'Rot13'")));
}

void TestListenerSettings::rejectsUnknownMode()
{
    const ListenerSettings settings = ListenerSettingsBuilder::build(
        { endpoint(QStringLiteral("a"), 4840, QStringLiteral("None"), QStringLiteral("Encrypt")) });
    QVERIFY(settings.error.contains(QStringLiteral("unknown security mode 'Encrypt'")));
}

void TestListenerSettings::securityPairs_data()
{
    QTest::addColumn<QString>("policy");
    QTest::addColumn<QString>("mode");
    QTest::addColumn<bool>("pairs");
    QTest::newRow("none") << "None" << "None" << true;
    QTest::newRow("sign") << "Basic256Sha256" << "Sign" << true;
    QTest::newRow("encrypt") << "Aes256_Sha256_RsaPss" << "Sign & Encrypt" << true;
    QTest::newRow("secure policy, no mode") << "Aes256_Sha256_RsaPss" << "None" << false;
    QTest::newRow("no policy, secure mode") << "None" << "Sign" << false;
}

void TestListenerSettings::securityPairs()
{
    QFETCH(QString, policy);
    QFETCH(QString, mode);
    QFETCH(bool, pairs);
    QCOMPARE(ListenerSettingsBuilder::securityPairs(policy, mode), pairs);
}

void TestListenerSettings::rejectsMismatchedPolicyAndMode()
{
    const ListenerSettings settings = ListenerSettingsBuilder::build(
        { endpoint(QStringLiteral("a"), 4840, QStringLiteral("Aes256_Sha256_RsaPss"), QStringLiteral("None")) });
    QVERIFY(settings.error.contains(
        QStringLiteral("pairs security policy 'Aes256_Sha256_RsaPss' with mode 'None'")));
}

void TestListenerSettings::usesSecurityOfEnabledEndpoints()
{
    EndpointInfo secure = endpoint(QStringLiteral("s"), 4840, QStringLiteral("Basic256Sha256"), QStringLiteral("Sign"));
    QVERIFY(!ListenerSettingsBuilder::usesSecurity({}));
    QVERIFY(!ListenerSettingsBuilder::usesSecurity({ endpoint(QStringLiteral("a")) }));
    QVERIFY(ListenerSettingsBuilder::usesSecurity({ endpoint(QStringLiteral("a")), secure }));
    secure.enabled = false;
    QVERIFY(!ListenerSettingsBuilder::usesSecurity({ secure }));
}

QTEST_GUILESS_MAIN(TestListenerSettings)

#include "test_listenersettings.moc"
