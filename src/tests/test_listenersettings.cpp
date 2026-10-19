// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QTest>

#include "server/listenersettings.h"

namespace {

EndpointInfo endpoint(const QString &name, int port = kDefaultEndpointPort, const QString &policy = QStringLiteral("None"),
                      const QString &mode = QStringLiteral("None"))
{
    EndpointInfo info;
    info.name = name;
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
    void collectsUsers();
    void anonymousWhenAnyEndpointAllowsIt();
    void rejectsNoEnabledEndpoint();
    void rejectsDifferentPorts();
    void rejectsInvalidPort();
    void rejectsUnknownPolicy();
    void rejectsUnknownMode();
    void rejectsUsernameEndpointWithoutUser();
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

void TestListenerSettings::collectsUsers()
{
    EndpointInfo secured = endpoint(QStringLiteral("a"));
    secured.usernamePassword = true;
    secured.username = QStringLiteral("operator");
    secured.password = QStringLiteral("secret");
    const ListenerSettings settings = ListenerSettingsBuilder::build({ secured });
    QCOMPARE(settings.users.size(), 1);
    QCOMPARE(settings.users.first().first, QStringLiteral("operator"));
    QCOMPARE(settings.users.first().second, QStringLiteral("secret"));
}

void TestListenerSettings::anonymousWhenAnyEndpointAllowsIt()
{
    EndpointInfo closed = endpoint(QStringLiteral("a"));
    closed.allowAnonymous = false;
    QVERIFY(!ListenerSettingsBuilder::build({ closed }).anonymousAllowed);
    QVERIFY(ListenerSettingsBuilder::build({ closed, endpoint(QStringLiteral("b")) }).anonymousAllowed);
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

void TestListenerSettings::rejectsUsernameEndpointWithoutUser()
{
    EndpointInfo secured = endpoint(QStringLiteral("a"));
    secured.usernamePassword = true;
    QVERIFY(ListenerSettingsBuilder::build({ secured }).error.contains(QStringLiteral("has none set")));
}

QTEST_GUILESS_MAIN(TestListenerSettings)

#include "test_listenersettings.moc"
