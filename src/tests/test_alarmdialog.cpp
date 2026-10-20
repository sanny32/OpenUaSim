// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTest>

#include <limits>

#include "alarmdialog.h"

namespace {

template <typename T>
T *child(const QWidget &parent, const char *name)
{
    T *found = parent.findChild<T *>(QLatin1String(name));
    if (!found)
        qFatal("Missing child %s", name);
    return found;
}

VariableInfo variable(const QString &dataType)
{
    VariableInfo info;
    info.displayName = QStringLiteral("Level");
    info.dataType = dataType;
    return info;
}

bool okEnabled(const AlarmDialog &dialog)
{
    return child<QDialogButtonBox>(dialog, "buttonBox")->button(QDialogButtonBox::Ok)->isEnabled();
}

}

class TestAlarmDialog : public QObject
{
    Q_OBJECT

private slots:
    void showsStoredSettings();
    void editsLimits();
    void refusesInvalidLimits();
    void fitsLimitsToDataType();
    void offersNormalValueForBooleans();
};

void TestAlarmDialog::showsStoredSettings()
{
    VariableInfo level = variable(QStringLiteral("Double"));
    level.alarm.enabled = true;
    level.alarm.highEnabled = true;
    level.alarm.high = 80.5;
    level.alarm.severity = 900;
    AlarmDialog dialog(level);
    QVERIFY(child<QGroupBox>(dialog, "alarmGroup")->isChecked());
    QVERIFY(child<QCheckBox>(dialog, "highCheck")->isChecked());
    QCOMPARE(child<QDoubleSpinBox>(dialog, "highSpin")->value(), 80.5);
    QCOMPARE(child<QSpinBox>(dialog, "severitySpin")->value(), 900);
    QVERIFY(child<QLabel>(dialog, "variableLabel")->text().contains(QStringLiteral("Level")));
    QVERIFY(okEnabled(dialog));
    QVERIFY(dialog.alarm() == level.alarm);
}

void TestAlarmDialog::editsLimits()
{
    AlarmDialog dialog(variable(QStringLiteral("Int32")));
    QVERIFY(!child<QGroupBox>(dialog, "alarmGroup")->isChecked());
    QVERIFY(!child<QDoubleSpinBox>(dialog, "lowLowSpin")->isEnabled());
    QVERIFY(okEnabled(dialog));

    child<QGroupBox>(dialog, "alarmGroup")->setChecked(true);
    QVERIFY(!okEnabled(dialog));
    QCOMPARE(child<QLabel>(dialog, "problemLabel")->text(), QStringLiteral("Enable at least one limit"));
    QVERIFY(!child<QDoubleSpinBox>(dialog, "lowLowSpin")->isEnabled());

    child<QCheckBox>(dialog, "lowLowCheck")->setChecked(true);
    QVERIFY(child<QDoubleSpinBox>(dialog, "lowLowSpin")->isEnabled());
    child<QDoubleSpinBox>(dialog, "lowLowSpin")->setValue(-5.0);
    QVERIFY(okEnabled(dialog));
    const AlarmSettings alarm = dialog.alarm();
    QVERIFY(alarm.enabled);
    QVERIFY(alarm.lowLowEnabled);
    QCOMPARE(alarm.lowLow, -5.0);
}

void TestAlarmDialog::refusesInvalidLimits()
{
    AlarmDialog dialog(variable(QStringLiteral("Double")));
    child<QGroupBox>(dialog, "alarmGroup")->setChecked(true);
    child<QCheckBox>(dialog, "lowCheck")->setChecked(true);
    child<QDoubleSpinBox>(dialog, "lowSpin")->setValue(50.0);
    child<QCheckBox>(dialog, "highCheck")->setChecked(true);
    child<QDoubleSpinBox>(dialog, "highSpin")->setValue(40.0);
    QVERIFY(!okEnabled(dialog));
    QVERIFY(child<QLabel>(dialog, "problemLabel")->text().contains(QStringLiteral("must rise")));
}

void TestAlarmDialog::fitsLimitsToDataType()
{
    AlarmDialog counter(variable(QStringLiteral("UInt32")));
    auto *counterSpin = child<QDoubleSpinBox>(counter, "highSpin");
    QCOMPARE(counterSpin->decimals(), 0);
    QCOMPARE(counterSpin->minimum(), 0.0);
    QCOMPARE(counterSpin->maximum(), 4294967295.0);

    AlarmDialog small(variable(QStringLiteral("SByte")));
    QCOMPARE(child<QDoubleSpinBox>(small, "lowSpin")->minimum(), -128.0);
    QCOMPARE(child<QDoubleSpinBox>(small, "lowSpin")->maximum(), 127.0);

    AlarmDialog level(variable(QStringLiteral("Double")));
    auto *levelSpin = child<QDoubleSpinBox>(level, "highSpin");
    QCOMPARE(levelSpin->decimals(), 3);
    QCOMPARE(levelSpin->minimum(), std::numeric_limits<double>::lowest());
}

void TestAlarmDialog::offersNormalValueForBooleans()
{
    AlarmDialog dialog(variable(QStringLiteral("Boolean")));
    dialog.show();
    QVERIFY(child<QComboBox>(dialog, "normalValueCombo")->isVisible());
    QVERIFY(!child<QCheckBox>(dialog, "highCheck")->isVisible());
    child<QGroupBox>(dialog, "alarmGroup")->setChecked(true);
    child<QComboBox>(dialog, "normalValueCombo")->setCurrentIndex(1);
    QVERIFY(okEnabled(dialog));
    QVERIFY(dialog.alarm().normalValue);
}

QTEST_MAIN(TestAlarmDialog)

#include "test_alarmdialog.moc"
