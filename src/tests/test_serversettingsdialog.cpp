// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

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

QTEST_MAIN(TestServerSettingsDialog)

#include "test_serversettingsdialog.moc"
