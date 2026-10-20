// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTest>

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
    QVERIFY(child<QCheckBox>(dialog, "enabledCheck")->isChecked());
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
    QVERIFY(!child<QWidget>(dialog, "settingsHost")->isEnabled());
    QVERIFY(okEnabled(dialog));

    child<QCheckBox>(dialog, "enabledCheck")->setChecked(true);
    QVERIFY(child<QWidget>(dialog, "settingsHost")->isEnabled());
    QVERIFY(!okEnabled(dialog));
    QCOMPARE(child<QLabel>(dialog, "problemLabel")->text(), QStringLiteral("Enable at least one limit"));

    child<QCheckBox>(dialog, "lowLowCheck")->setChecked(true);
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
    child<QCheckBox>(dialog, "enabledCheck")->setChecked(true);
    child<QCheckBox>(dialog, "lowCheck")->setChecked(true);
    child<QDoubleSpinBox>(dialog, "lowSpin")->setValue(50.0);
    child<QCheckBox>(dialog, "highCheck")->setChecked(true);
    child<QDoubleSpinBox>(dialog, "highSpin")->setValue(40.0);
    QVERIFY(!okEnabled(dialog));
    QVERIFY(child<QLabel>(dialog, "problemLabel")->text().contains(QStringLiteral("must rise")));
}

void TestAlarmDialog::offersNormalValueForBooleans()
{
    AlarmDialog dialog(variable(QStringLiteral("Boolean")));
    dialog.show();
    QVERIFY(child<QComboBox>(dialog, "normalValueCombo")->isVisible());
    QVERIFY(!child<QCheckBox>(dialog, "highCheck")->isVisible());
    child<QCheckBox>(dialog, "enabledCheck")->setChecked(true);
    child<QComboBox>(dialog, "normalValueCombo")->setCurrentIndex(1);
    QVERIFY(okEnabled(dialog));
    QVERIFY(dialog.alarm().normalValue);
}

QTEST_MAIN(TestAlarmDialog)

#include "test_alarmdialog.moc"
