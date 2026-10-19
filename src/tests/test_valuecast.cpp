// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QDateTime>
#include <QTimeZone>
#include <QTest>

#include <limits>

#include "simulation/valuecast.h"

class TestValueCast : public QObject
{
    Q_OBJECT

private slots:
    void convertsToType_data();
    void convertsToType();
    void nanBecomesZero();
    void dateTimeCountsEpochSeconds();
    void unsupportedTypeIsInvalid();
    void defaultValues();
    void listsSupportedTypes();
    void convertsAnyValue_data();
    void convertsAnyValue();
    void reportsMetaType();
    void parsesText_data();
    void parsesText();
    void rejectsInvalidText_data();
    void rejectsInvalidText();
};

void TestValueCast::convertsAnyValue_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QString>("dataType");
    QTest::addColumn<QVariant>("expected");

    QTest::newRow("double rounds to int") << QVariant(12.7) << "Int32" << QVariant::fromValue(qint32(13));
    QTest::newRow("int saturates byte") << QVariant(300) << "Byte" << QVariant::fromValue(quint8(255));
    QTest::newRow("bool to double") << QVariant(true) << "Double" << QVariant(1.0);
    QTest::newRow("numeric string to int") << QVariant(QStringLiteral("42")) << "Int16"
                                           << QVariant::fromValue(qint16(42));
    QTest::newRow("text to string") << QVariant(QStringLiteral("abc")) << "String" << QVariant(QStringLiteral("abc"));
    QTest::newRow("invalid text falls back") << QVariant(QStringLiteral("abc")) << "Double" << QVariant(0.0);
    QTest::newRow("invalid variant falls back") << QVariant() << "Boolean" << QVariant(false);
}

void TestValueCast::convertsAnyValue()
{
    QFETCH(QVariant, value);
    QFETCH(QString, dataType);
    QFETCH(QVariant, expected);

    const QVariant actual = ValueCast::convert(value, dataType);
    QCOMPARE(actual.metaType(), expected.metaType());
    QCOMPARE(actual, expected);
}

void TestValueCast::reportsMetaType()
{
    QCOMPARE(ValueCast::metaType(QStringLiteral("UInt16")), QMetaType::UShort);
    QCOMPARE(ValueCast::metaType(QStringLiteral("String")), QMetaType::QString);
    QCOMPARE(ValueCast::metaType(QStringLiteral("Guid")), QMetaType::UnknownType);
}

void TestValueCast::convertsToType_data()
{
    QTest::addColumn<double>("value");
    QTest::addColumn<QString>("dataType");
    QTest::addColumn<QVariant>("expected");

    QTest::newRow("boolean rounds to true") << 0.6 << "Boolean" << QVariant(true);
    QTest::newRow("boolean rounds to false") << 0.4 << "Boolean" << QVariant(false);
    QTest::newRow("sbyte saturates high") << 1000.0 << "SByte" << QVariant::fromValue(qint8(127));
    QTest::newRow("sbyte saturates low") << -1000.0 << "SByte" << QVariant::fromValue(qint8(-128));
    QTest::newRow("byte clamps negative") << -5.0 << "Byte" << QVariant::fromValue(quint8(0));
    QTest::newRow("int16 rounds") << 12.5 << "Int16" << QVariant::fromValue(qint16(13));
    QTest::newRow("uint16 saturates") << 1e9 << "UInt16" << QVariant::fromValue(quint16(65535));
    QTest::newRow("int32 rounds negative") << -2.5 << "Int32" << QVariant::fromValue(qint32(-3));
    QTest::newRow("uint32 clamps negative") << -1.0 << "UInt32" << QVariant::fromValue(quint32(0));
    QTest::newRow("int64 saturates") << 1e30 << "Int64"
                                     << QVariant::fromValue(std::numeric_limits<qint64>::max());
    QTest::newRow("uint64 saturates") << 1e30 << "UInt64"
                                      << QVariant::fromValue(std::numeric_limits<quint64>::max());
    QTest::newRow("float") << 1.5 << "Float" << QVariant::fromValue(1.5f);
    QTest::newRow("double") << 1.25 << "Double" << QVariant::fromValue(1.25);
    QTest::newRow("string") << 3.14159265 << "String" << QVariant(QStringLiteral("3.14159"));
}

void TestValueCast::convertsToType()
{
    QFETCH(double, value);
    QFETCH(QString, dataType);
    QFETCH(QVariant, expected);

    const QVariant actual = ValueCast::fromDouble(value, dataType);
    QCOMPARE(actual.metaType(), expected.metaType());
    QCOMPARE(actual, expected);
}

void TestValueCast::nanBecomesZero()
{
    QCOMPARE(ValueCast::fromDouble(qQNaN(), QStringLiteral("Int32")), QVariant::fromValue(qint32(0)));
}

void TestValueCast::dateTimeCountsEpochSeconds()
{
    const QDateTime time = ValueCast::fromDouble(60.0, QStringLiteral("DateTime")).toDateTime();
    QCOMPARE(time, QDateTime(QDate(1970, 1, 1), QTime(0, 1), QTimeZone::UTC));
}

void TestValueCast::unsupportedTypeIsInvalid()
{
    QVERIFY(!ValueCast::fromDouble(1.0, QStringLiteral("Guid")).isValid());
    QVERIFY(!ValueCast::defaultValue(QStringLiteral("Guid")).isValid());
    QVERIFY(!ValueCast::isSupported(QStringLiteral("Guid")));
}

void TestValueCast::defaultValues()
{
    QCOMPARE(ValueCast::defaultValue(QStringLiteral("Boolean")), QVariant(false));
    QCOMPARE(ValueCast::defaultValue(QStringLiteral("Double")), QVariant::fromValue(0.0));
    QCOMPARE(ValueCast::defaultValue(QStringLiteral("String")), QVariant(QString()));
}

void TestValueCast::listsSupportedTypes()
{
    const QStringList types = ValueCast::dataTypes();
    QCOMPARE(types.size(), 13);
    for (const QString &type : types) {
        QVERIFY(ValueCast::isSupported(type));
        QVERIFY2(ValueCast::fromDouble(1.0, type).isValid(), qPrintable(type));
    }
}

void TestValueCast::parsesText_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("dataType");
    QTest::addColumn<QVariant>("expected");

    QTest::newRow("Boolean true") << "TRUE" << "Boolean" << QVariant(true);
    QTest::newRow("Boolean 0") << " 0 " << "Boolean" << QVariant(false);
    QTest::newRow("SByte min") << "-128" << "SByte" << QVariant::fromValue(qint8(-128));
    QTest::newRow("Byte max") << "255" << "Byte" << QVariant::fromValue(quint8(255));
    QTest::newRow("Int32") << "-42" << "Int32" << QVariant::fromValue(qint32(-42));
    QTest::newRow("UInt64 max") << "18446744073709551615" << "UInt64"
                                << QVariant::fromValue(std::numeric_limits<quint64>::max());
    QTest::newRow("Float") << "2.5" << "Float" << QVariant::fromValue(2.5f);
    QTest::newRow("Double") << "1e3" << "Double" << QVariant(1000.0);
    QTest::newRow("String keeps spaces") << " a b " << "String" << QVariant(QStringLiteral(" a b "));
    QTest::newRow("DateTime") << "2026-01-02T03:04:05.006Z" << "DateTime"
                              << QVariant(QDateTime(QDate(2026, 1, 2), QTime(3, 4, 5, 6), QTimeZone::UTC));
}

void TestValueCast::parsesText()
{
    QFETCH(QString, text);
    QFETCH(QString, dataType);
    QFETCH(QVariant, expected);

    const QVariant parsed = ValueCast::fromText(text, dataType);
    QCOMPARE(parsed.metaType(), expected.metaType());
    QCOMPARE(parsed, expected);
}

void TestValueCast::rejectsInvalidText_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("dataType");

    QTest::newRow("Boolean word") << "yes" << "Boolean";
    QTest::newRow("SByte overflow") << "128" << "SByte";
    QTest::newRow("Byte negative") << "-1" << "Byte";
    QTest::newRow("Int32 fraction") << "1.5" << "Int32";
    QTest::newRow("Int16 empty") << "" << "Int16";
    QTest::newRow("Float overflow") << "1e39" << "Float";
    QTest::newRow("Double word") << "abc" << "Double";
    QTest::newRow("DateTime word") << "yesterday" << "DateTime";
    QTest::newRow("unsupported") << "1" << "Guid";
}

void TestValueCast::rejectsInvalidText()
{
    QFETCH(QString, text);
    QFETCH(QString, dataType);

    QVERIFY(!ValueCast::fromText(text, dataType).isValid());
}

QTEST_GUILESS_MAIN(TestValueCast)

#include "test_valuecast.moc"
