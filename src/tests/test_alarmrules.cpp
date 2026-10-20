// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QTest>

#include "project/alarmrules.h"

Q_DECLARE_METATYPE(AlarmSettings)

namespace {

AlarmSettings limits(bool lowEnabled, double low, bool highEnabled, double high)
{
    AlarmSettings alarm;
    alarm.enabled = true;
    alarm.lowEnabled = lowEnabled;
    alarm.low = low;
    alarm.highEnabled = highEnabled;
    alarm.high = high;
    return alarm;
}

}

class TestAlarmRules : public QObject
{
    Q_OBJECT

private slots:
    void acceptsDisabledAlarmOnAnyVariable();
    void picksOffNormalForBooleans();
    void acceptsValidSettings_data();
    void acceptsValidSettings();
    void rejectsInvalidSettings_data();
    void rejectsInvalidSettings();
};

void TestAlarmRules::acceptsDisabledAlarmOnAnyVariable()
{
    QVERIFY(AlarmRules::problem(AlarmSettings(), QStringLiteral("String"), 4).isEmpty());
}

void TestAlarmRules::picksOffNormalForBooleans()
{
    QVERIFY(AlarmRules::isOffNormal(QStringLiteral("Boolean")));
    QVERIFY(!AlarmRules::isOffNormal(QStringLiteral("Double")));
}

void TestAlarmRules::acceptsValidSettings_data()
{
    QTest::addColumn<AlarmSettings>("alarm");
    QTest::addColumn<QString>("dataType");

    QTest::newRow("high only") << limits(false, 0.0, true, 80.0) << "Double";
    QTest::newRow("low and high") << limits(true, 10.0, true, 80.0) << "Int16";
    AlarmSettings four = limits(true, 10.0, true, 80.0);
    four.lowLowEnabled = true;
    four.lowLow = 5.0;
    four.highHighEnabled = true;
    four.highHigh = 95.0;
    QTest::newRow("all four") << four << "Float";
    AlarmSettings offNormal;
    offNormal.enabled = true;
    QTest::newRow("Boolean without limits") << offNormal << "Boolean";
}

void TestAlarmRules::acceptsValidSettings()
{
    QFETCH(AlarmSettings, alarm);
    QFETCH(QString, dataType);
    QCOMPARE(AlarmRules::problem(alarm, dataType, 0), QString());
}

void TestAlarmRules::rejectsInvalidSettings_data()
{
    QTest::addColumn<AlarmSettings>("alarm");
    QTest::addColumn<QString>("dataType");
    QTest::addColumn<int>("arrayLength");
    QTest::addColumn<QString>("error");

    const AlarmSettings high = limits(false, 0.0, true, 80.0);
    QTest::newRow("array") << high << "Double" << 3 << "An alarm needs a scalar variable";
    QTest::newRow("string") << high << "String" << 0 << "An alarm needs a numeric or Boolean variable";
    QTest::newRow("date") << high << "DateTime" << 0 << "An alarm needs a numeric or Boolean variable";
    QTest::newRow("no limit") << limits(false, 0.0, false, 0.0) << "Double" << 0 << "Enable at least one limit";
    QTest::newRow("crossed limits") << limits(true, 90.0, true, 80.0) << "Double" << 0
                                    << "The limits must rise from LowLow through Low and High to HighHigh";
    AlarmSettings severity = high;
    severity.severity = 0;
    QTest::newRow("severity") << severity << "Double" << 0 << "The severity must be between 1 and 1000";
}

void TestAlarmRules::rejectsInvalidSettings()
{
    QFETCH(AlarmSettings, alarm);
    QFETCH(QString, dataType);
    QFETCH(int, arrayLength);
    QFETCH(QString, error);
    QCOMPARE(AlarmRules::problem(alarm, dataType, arrayLength), error);
}

QTEST_GUILESS_MAIN(TestAlarmRules)

#include "test_alarmrules.moc"
