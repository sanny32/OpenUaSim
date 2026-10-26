// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTest>

#include "serversettingsdialog.h"

namespace {

template <typename T>
T *child(const QWidget &parent, const char *name)
{
    T *found = parent.findChild<T *>(QLatin1String(name));
    if (!found)
        qFatal("Missing child %s", name);
    return found;
}

QPushButton *okButton(const ServerSettingsDialog &dialog)
{
    return child<QDialogButtonBox>(dialog, "buttonBox")->button(QDialogButtonBox::Ok);
}

}

class TestServerSettingsDialog : public QObject
{
    Q_OBJECT

private slots:
    void showsSettings();
    void returnsTrimmedSettings();
    void refusesEmptyFields();
    void limitsSessionCount();
    void editsHistoryLimits();
    void editsDiscoveryRegistration();
    void validatesDiscoveryRegistration_data();
    void validatesDiscoveryRegistration();
};

void TestServerSettingsDialog::showsSettings()
{
    ServerSettingsDialog dialog({ QStringLiteral("Sim"), QStringLiteral("urn:sim"), 12, { 500, 15 } });
    QCOMPARE(child<QLineEdit>(dialog, "applicationNameEdit")->text(), QStringLiteral("Sim"));
    QCOMPARE(child<QLineEdit>(dialog, "applicationUriEdit")->text(), QStringLiteral("urn:sim"));
    QCOMPARE(child<QSpinBox>(dialog, "maxSessionsSpin")->value(), 12);
    QCOMPARE(child<QSpinBox>(dialog, "historyMaxValuesSpin")->value(), 500);
    QCOMPARE(child<QSpinBox>(dialog, "historyMaxAgeSpin")->value(), 15);
    QVERIFY(okButton(dialog)->isEnabled());
    QVERIFY(child<QLabel>(dialog, "problemLabel")->text().isEmpty());
}

void TestServerSettingsDialog::returnsTrimmedSettings()
{
    ServerSettingsDialog dialog(ServerSettings {});
    child<QLineEdit>(dialog, "applicationNameEdit")->setText(QStringLiteral("  Line  "));
    child<QLineEdit>(dialog, "applicationUriEdit")->setText(QStringLiteral(" urn:line "));
    child<QSpinBox>(dialog, "maxSessionsSpin")->setValue(3);

    const ServerSettings settings = dialog.settings();
    QCOMPARE(settings.applicationName, QStringLiteral("Line"));
    QCOMPARE(settings.applicationUri, QStringLiteral("urn:line"));
    QCOMPARE(settings.maxSessions, 3);
}

void TestServerSettingsDialog::refusesEmptyFields()
{
    ServerSettingsDialog dialog(ServerSettings {});
    auto *name = child<QLineEdit>(dialog, "applicationNameEdit");
    name->setText(QStringLiteral(" "));
    QVERIFY(!okButton(dialog)->isEnabled());
    QCOMPARE(child<QLabel>(dialog, "problemLabel")->text(), QStringLiteral("The application name is empty"));

    name->setText(QStringLiteral("Sim"));
    child<QLineEdit>(dialog, "applicationUriEdit")->clear();
    QVERIFY(!okButton(dialog)->isEnabled());
    QCOMPARE(child<QLabel>(dialog, "problemLabel")->text(), QStringLiteral("The application URI is empty"));
}

void TestServerSettingsDialog::limitsSessionCount()
{
    ServerSettingsDialog dialog(ServerSettings {});
    auto *sessions = child<QSpinBox>(dialog, "maxSessionsSpin");
    QCOMPARE(sessions->minimum(), 1);
    QCOMPARE(sessions->maximum(), kMaxSessionLimit);
}

void TestServerSettingsDialog::editsHistoryLimits()
{
    ServerSettingsDialog dialog(ServerSettings {});
    auto *values = child<QSpinBox>(dialog, "historyMaxValuesSpin");
    auto *age = child<QSpinBox>(dialog, "historyMaxAgeSpin");
    QCOMPARE(values->minimum(), 1);
    QCOMPARE(values->maximum(), kMaxHistoryValues);
    QCOMPARE(age->minimum(), 0);
    QCOMPARE(age->maximum(), kMaxHistoryAgeMinutes);
    QCOMPARE(values->value(), HistoryLimits().maxValues);
    QCOMPARE(age->text(), QStringLiteral("Unlimited"));

    values->setValue(2500);
    age->setValue(60);
    const HistoryLimits history = dialog.settings().history;
    QCOMPARE(history.maxValues, 2500);
    QCOMPARE(history.maxAgeMinutes, 60);
}

void TestServerSettingsDialog::editsDiscoveryRegistration()
{
    ServerSettingsDialog dialog(ServerSettings {});
    auto *check = child<QCheckBox>(dialog, "discoveryCheck");
    auto *url = child<QLineEdit>(dialog, "discoveryUrlEdit");
    auto *interval = child<QSpinBox>(dialog, "discoveryIntervalSpin");
    QVERIFY(!check->isChecked());
    QVERIFY(!url->isEnabled());
    QVERIFY(!interval->isEnabled());
    QCOMPARE(url->text(), DiscoveryRegistration().url);
    QCOMPARE(interval->minimum(), kMinDiscoveryIntervalSeconds);
    QCOMPARE(interval->maximum(), kMaxDiscoveryIntervalSeconds);

    check->setChecked(true);
    QVERIFY(url->isEnabled());
    QVERIFY(interval->isEnabled());
    url->setText(QStringLiteral(" opc.tcp://lds.plant:4840 "));
    interval->setValue(90);
    const DiscoveryRegistration discovery = dialog.settings().discovery;
    QVERIFY(discovery.enabled);
    QCOMPARE(discovery.url, QStringLiteral("opc.tcp://lds.plant:4840"));
    QCOMPARE(discovery.intervalSeconds, 90);
}

void TestServerSettingsDialog::validatesDiscoveryRegistration_data()
{
    QTest::addColumn<bool>("enabled");
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("accepted");

    QTest::newRow("local discovery server") << true << QStringLiteral("opc.tcp://localhost:4840") << true;
    QTest::newRow("without port") << true << QStringLiteral("opc.tcp://lds.plant") << true;
    QTest::newRow("other scheme") << true << QStringLiteral("http://localhost:4840") << false;
    QTest::newRow("no host") << true << QStringLiteral("opc.tcp://:4840") << false;
    QTest::newRow("empty") << true << QString() << false;
    QTest::newRow("disabled keeps a bad URL") << false << QStringLiteral("lds") << true;
}

void TestServerSettingsDialog::validatesDiscoveryRegistration()
{
    QFETCH(bool, enabled);
    QFETCH(QString, url);
    QFETCH(bool, accepted);

    ServerSettingsDialog dialog(ServerSettings {});
    child<QCheckBox>(dialog, "discoveryCheck")->setChecked(enabled);
    child<QLineEdit>(dialog, "discoveryUrlEdit")->setText(url);
    QCOMPARE(okButton(dialog)->isEnabled(), accepted);
    QCOMPARE(child<QLabel>(dialog, "problemLabel")->text(),
             accepted ? QString() : QStringLiteral("The discovery server URL must look like opc.tcp://host:port"));
}

QTEST_MAIN(TestServerSettingsDialog)

#include "test_serversettingsdialog.moc"
