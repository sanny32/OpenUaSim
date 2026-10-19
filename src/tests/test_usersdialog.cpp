// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTest>

#include "usersdialog.h"

namespace {

template <typename T>
T *child(const QWidget &parent, const char *name)
{
    T *found = parent.findChild<T *>(QLatin1String(name));
    if (!found)
        qFatal("Missing child %s", name);
    return found;
}

QPushButton *okButton(const UsersDialog &dialog)
{
    return child<QDialogButtonBox>(dialog, "buttonBox")->button(QDialogButtonBox::Ok);
}

}

class TestUsersDialog : public QObject
{
    Q_OBJECT

private slots:
    void showsAccounts();
    void addsAndRemovesAccounts();
    void refusesInvalidAccounts();
};

void TestUsersDialog::showsAccounts()
{
    UsersDialog dialog({ { QStringLiteral("operator"), QStringLiteral("secret") } });
    auto *table = child<QTableWidget>(dialog, "usersTable");
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 0)->text(), QStringLiteral("operator"));
    QCOMPARE(table->item(0, 1)->text(), QStringLiteral("secret"));
    QVERIFY(okButton(dialog)->isEnabled());
}

void TestUsersDialog::addsAndRemovesAccounts()
{
    UsersDialog dialog({});
    auto *table = child<QTableWidget>(dialog, "usersTable");
    QVERIFY(!child<QPushButton>(dialog, "removeUserButton")->isEnabled());

    child<QPushButton>(dialog, "addUserButton")->click();
    QCOMPARE(table->rowCount(), 1);
    table->item(0, 0)->setText(QStringLiteral(" engineer "));
    table->item(0, 1)->setText(QStringLiteral("pw"));
    QVERIFY(okButton(dialog)->isEnabled());
    const QList<UserAccount> users = dialog.users();
    QCOMPARE(users.size(), 1);
    QCOMPARE(users.first().username, QStringLiteral("engineer"));
    QCOMPARE(users.first().password, QStringLiteral("pw"));

    table->setCurrentCell(0, 0);
    child<QPushButton>(dialog, "removeUserButton")->click();
    QCOMPARE(table->rowCount(), 0);
    QVERIFY(dialog.users().isEmpty());
    QVERIFY(okButton(dialog)->isEnabled());
}

void TestUsersDialog::refusesInvalidAccounts()
{
    UsersDialog dialog({ { QStringLiteral("operator"), QString() } });
    child<QPushButton>(dialog, "addUserButton")->click();
    QVERIFY(!okButton(dialog)->isEnabled());
    QCOMPARE(child<QLabel>(dialog, "problemLabel")->text(), QStringLiteral("A user account has no user name"));

    child<QTableWidget>(dialog, "usersTable")->item(1, 0)->setText(QStringLiteral("operator"));
    QVERIFY(!okButton(dialog)->isEnabled());
    QCOMPARE(child<QLabel>(dialog, "problemLabel")->text(),
             QStringLiteral("The user name 'operator' is used more than once"));
}

QTEST_MAIN(TestUsersDialog)

#include "test_usersdialog.moc"
