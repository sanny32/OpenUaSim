// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QTest>

#include "project/serversettingsrules.h"

Q_DECLARE_METATYPE(ServerSettings)

namespace {

ServerSettings withDiscovery(const QString &url, int intervalSeconds)
{
    ServerSettings settings;
    settings.discovery.enabled = true;
    settings.discovery.url = url;
    settings.discovery.intervalSeconds = intervalSeconds;
    return settings;
}

}

class TestServerSettingsRules : public QObject
{
    Q_OBJECT

private slots:
    void acceptsValidSettings_data();
    void acceptsValidSettings();
    void rejectsInvalidSettings_data();
    void rejectsInvalidSettings();
};

void TestServerSettingsRules::acceptsValidSettings_data()
{
    QTest::addColumn<ServerSettings>("settings");

    QTest::newRow("defaults") << ServerSettings();

    ServerSettings bounds;
    bounds.maxSessions = 1;
    bounds.history = { 1, 0 };
    QTest::newRow("lower bounds") << bounds;
    bounds.maxSessions = kMaxSessionLimit;
    bounds.history = { kMaxHistoryValues, kMaxHistoryAgeMinutes };
    QTest::newRow("upper bounds") << bounds;

    ServerSettings disabled = withDiscovery(QStringLiteral("not a url"), 0);
    disabled.discovery.enabled = false;
    QTest::newRow("disabled discovery is not checked") << disabled;
    QTest::newRow("discovery") << withDiscovery(QStringLiteral(" opc.tcp://lds.local:4840 "),
                                                kMinDiscoveryIntervalSeconds);
    QTest::newRow("longest interval") << withDiscovery(QStringLiteral("opc.tcp://10.0.0.1"),
                                                       kMaxDiscoveryIntervalSeconds);
}

void TestServerSettingsRules::acceptsValidSettings()
{
    QFETCH(ServerSettings, settings);
    QCOMPARE(ServerSettingsRules::problem(settings), QString());
}

void TestServerSettingsRules::rejectsInvalidSettings_data()
{
    QTest::addColumn<ServerSettings>("settings");
    QTest::addColumn<QString>("problem");

    ServerSettings settings;
    settings.applicationName = QStringLiteral("  ");
    QTest::newRow("blank name") << settings << QStringLiteral("The application name is empty");

    settings = ServerSettings();
    settings.applicationUri.clear();
    QTest::newRow("empty URI") << settings << QStringLiteral("The application URI is empty");

    const QString sessions = QStringLiteral("The session limit must be between 1 and %1").arg(kMaxSessionLimit);
    settings = ServerSettings();
    settings.maxSessions = 0;
    QTest::newRow("no sessions") << settings << sessions;
    settings.maxSessions = kMaxSessionLimit + 1;
    QTest::newRow("too many sessions") << settings << sessions;

    const QString values = QStringLiteral("The history must keep between 1 and %1 values").arg(kMaxHistoryValues);
    settings = ServerSettings();
    settings.history.maxValues = 0;
    QTest::newRow("no history values") << settings << values;
    settings.history.maxValues = kMaxHistoryValues + 1;
    QTest::newRow("too many history values") << settings << values;

    const QString age =
        QStringLiteral("The history retention must be between 0 and %1 minutes").arg(kMaxHistoryAgeMinutes);
    settings = ServerSettings();
    settings.history.maxAgeMinutes = -1;
    QTest::newRow("negative retention") << settings << age;
    settings.history.maxAgeMinutes = kMaxHistoryAgeMinutes + 1;
    QTest::newRow("too long retention") << settings << age;

    const QString url = QStringLiteral("The discovery server URL must look like opc.tcp://host:port");
    QTest::newRow("empty URL") << withDiscovery(QString(), kMinDiscoveryIntervalSeconds) << url;
    QTest::newRow("other scheme") << withDiscovery(QStringLiteral("http://lds:4840"), kMinDiscoveryIntervalSeconds)
                                  << url;
    QTest::newRow("no host") << withDiscovery(QStringLiteral("opc.tcp://:4840"), kMinDiscoveryIntervalSeconds) << url;

    const QString interval = QStringLiteral("The registration interval must be between %1 and %2 seconds")
                                 .arg(kMinDiscoveryIntervalSeconds)
                                 .arg(kMaxDiscoveryIntervalSeconds);
    QTest::newRow("short interval") << withDiscovery(QStringLiteral("opc.tcp://lds:4840"),
                                                     kMinDiscoveryIntervalSeconds - 1)
                                    << interval;
    QTest::newRow("long interval") << withDiscovery(QStringLiteral("opc.tcp://lds:4840"),
                                                    kMaxDiscoveryIntervalSeconds + 1)
                                   << interval;
}

void TestServerSettingsRules::rejectsInvalidSettings()
{
    QFETCH(ServerSettings, settings);
    QFETCH(QString, problem);
    QCOMPARE(ServerSettingsRules::problem(settings), problem);
}

QTEST_GUILESS_MAIN(TestServerSettingsRules)
#include "test_serversettingsrules.moc"
