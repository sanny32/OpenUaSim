// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QTest>

#include "project/useraccountrules.h"

Q_DECLARE_METATYPE(QList<UserAccount>)

namespace {

UserAccount account(const QString &username, const QString &password = QStringLiteral("secret"))
{
    return { username, password };
}

}

class TestUserAccountRules : public QObject
{
    Q_OBJECT

private slots:
    void acceptsValidAccounts_data();
    void acceptsValidAccounts();
    void rejectsInvalidAccounts_data();
    void rejectsInvalidAccounts();
};

void TestUserAccountRules::acceptsValidAccounts_data()
{
    QTest::addColumn<QList<UserAccount>>("users");

    QTest::newRow("none") << QList<UserAccount>();
    QTest::newRow("one") << QList<UserAccount>({ account(QStringLiteral("operator")) });
    QTest::newRow("distinct") << QList<UserAccount>({ account(QStringLiteral("operator")),
                                                      account(QStringLiteral("engineer")) });
    QTest::newRow("empty password") << QList<UserAccount>({ account(QStringLiteral("guest"), QString()) });
    QTest::newRow("names differ in case") << QList<UserAccount>({ account(QStringLiteral("Operator")),
                                                                  account(QStringLiteral("operator")) });
}

void TestUserAccountRules::acceptsValidAccounts()
{
    QFETCH(QList<UserAccount>, users);
    QCOMPARE(UserAccountRules::problem(users), QString());
}

void TestUserAccountRules::rejectsInvalidAccounts_data()
{
    QTest::addColumn<QList<UserAccount>>("users");
    QTest::addColumn<QString>("problem");

    QTest::newRow("empty name") << QList<UserAccount>({ account(QString()) })
                                << QStringLiteral("A user account has no user name");
    QTest::newRow("blank name") << QList<UserAccount>({ account(QStringLiteral("operator")),
                                                        account(QStringLiteral("  ")) })
                                << QStringLiteral("A user account has no user name");
    QTest::newRow("duplicate") << QList<UserAccount>({ account(QStringLiteral("operator")),
                                                       account(QStringLiteral("engineer")),
                                                       account(QStringLiteral("operator"), QStringLiteral("other")) })
                               << QStringLiteral("The user name 'operator' is used more than once");
}

void TestUserAccountRules::rejectsInvalidAccounts()
{
    QFETCH(QList<UserAccount>, users);
    QFETCH(QString, problem);
    QCOMPARE(UserAccountRules::problem(users), problem);
}

QTEST_GUILESS_MAIN(TestUserAccountRules)
#include "test_useraccountrules.moc"
