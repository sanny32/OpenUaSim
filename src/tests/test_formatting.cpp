// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QLocale>
#include <QTest>
#include <QTimeZone>

#include "formatting.h"

class TestFormatting : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void dateTimeDropsMilliseconds();
    void preciseDateTimeKeepsMilliseconds();
    void dateTimeShowsUtcInLocalTime();
    void duration_data();
    void duration();
    void value_data();
    void value();
};

void TestFormatting::initTestCase()
{
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
}

void TestFormatting::dateTimeDropsMilliseconds()
{
    const QDateTime time(QDate(2024, 5, 23), QTime(14, 31, 20, 4));
    QCOMPARE(Formatting::dateTime(time), QStringLiteral("2024-05-23 14:31:20"));
}

void TestFormatting::preciseDateTimeKeepsMilliseconds()
{
    const QDateTime time(QDate(2024, 5, 23), QTime(14, 31, 20, 4));
    QCOMPARE(Formatting::preciseDateTime(time), QStringLiteral("2024-05-23 14:31:20.004"));
}

void TestFormatting::dateTimeShowsUtcInLocalTime()
{
    const QDateTime utc(QDate(2024, 5, 23), QTime(14, 31, 20, 4), QTimeZone::UTC);
    const QDateTime local = utc.toLocalTime();
    QCOMPARE(Formatting::dateTime(utc), local.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
    QCOMPARE(Formatting::preciseDateTime(utc), local.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")));
}

void TestFormatting::duration_data()
{
    QTest::addColumn<qint64>("seconds");
    QTest::addColumn<QString>("expected");

    QTest::newRow("zero") << qint64(0) << QStringLiteral("00:00:00");
    QTest::newRow("negative is clamped") << qint64(-5) << QStringLiteral("00:00:00");
    QTest::newRow("minutes and seconds") << qint64(27 * 60 + 36) << QStringLiteral("00:27:36");
    QTest::newRow("hours beyond two digits") << qint64(123 * 3600 + 1) << QStringLiteral("123:00:01");
}

void TestFormatting::duration()
{
    QFETCH(qint64, seconds);
    QFETCH(QString, expected);
    QCOMPARE(Formatting::duration(seconds), expected);
}

void TestFormatting::value_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QString>("expected");

    QTest::newRow("double with group separator") << QVariant(1487.62) << QStringLiteral("1,487.62");
    QTest::newRow("double keeps every digit") << QVariant(1405.7971878156036)
                                              << QStringLiteral("1,405.7971878156036");
    QTest::newRow("whole double") << QVariant(32.0) << QStringLiteral("32");
    QTest::newRow("float") << QVariant(2.5f) << QStringLiteral("2.5");
    QTest::newRow("true") << QVariant(true) << QStringLiteral("true");
    QTest::newRow("false") << QVariant(false) << QStringLiteral("false");
    QTest::newRow("integer") << QVariant(42) << QStringLiteral("42");
    QTest::newRow("array") << QVariant(QVariantList { 1.5, true, 3 }) << QStringLiteral("[1.5, true, 3]");
    QTest::newRow("typed array") << QVariant::fromValue(QList<double> { 1000.0, 2.5 })
                                 << QStringLiteral("[1,000, 2.5]");
    QTest::newRow("empty array") << QVariant(QVariantList()) << QStringLiteral("[]");
    QTest::newRow("string") << QVariant(QStringLiteral("text")) << QStringLiteral("text");
    QTest::newRow("invalid") << QVariant() << QString();
}

void TestFormatting::value()
{
    QFETCH(QVariant, value);
    QFETCH(QString, expected);
    QCOMPARE(Formatting::value(value), expected);
}

QTEST_GUILESS_MAIN(TestFormatting)
#include "test_formatting.moc"
