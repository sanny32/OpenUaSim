// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QFileInfo>
#include <QTest>

#include "settingsstore.h"

class TestSettingsStore : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void storesUnderProductName();
    void keepsValuesAcrossInstances();
    void ignoresOrganizationAndApplication();
};

void TestSettingsStore::init()
{
    SettingsStore().clear();
}

void TestSettingsStore::storesUnderProductName()
{
    const SettingsStore settings;
    QCOMPARE(settings.format(), QSettings::IniFormat);
    QCOMPARE(QFileInfo(settings.fileName()).completeBaseName(), QStringLiteral(APP_NAME));
}

void TestSettingsStore::keepsValuesAcrossInstances()
{
    SettingsStore().setValue(QStringLiteral("group/key"), 42);
    QCOMPARE(SettingsStore().value(QStringLiteral("group/key")).toInt(), 42);
    QCOMPARE(SettingsStore().childGroups(), QStringList { QStringLiteral("group") });
}

void TestSettingsStore::ignoresOrganizationAndApplication()
{
    SettingsStore().setValue(QStringLiteral("key"), true);
    QVERIFY(!QSettings().contains(QStringLiteral("key")));
    QVERIFY(QSettings().fileName() != SettingsStore().fileName());
}

QTEST_GUILESS_MAIN(TestSettingsStore)

#include "test_settingsstore.moc"
