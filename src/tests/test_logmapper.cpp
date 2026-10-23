// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QTest>

#include <QUaServer>

#include "server/logmapper.h"

Q_DECLARE_METATYPE(LogEntry::Level)
Q_DECLARE_METATYPE(LogEntry::Category)

class TestLogMapper : public QObject
{
    Q_OBJECT

private slots:
    void mapsLevel_data();
    void mapsLevel();
    void dropsTraceAndDebug();
    void mapsCategory_data();
    void mapsCategory();
    void keepsMessageAndTime();
    void buildsOwnEntries();
};

void TestLogMapper::mapsLevel_data()
{
    QTest::addColumn<QUaLogLevel>("input");
    QTest::addColumn<LogEntry::Level>("expected");

    QTest::newRow("info") << QUaLogLevel::Info << LogEntry::Level::Info;
    QTest::newRow("warning") << QUaLogLevel::Warning << LogEntry::Level::Warning;
    QTest::newRow("error") << QUaLogLevel::Error << LogEntry::Level::Error;
    QTest::newRow("fatal") << QUaLogLevel::Fatal << LogEntry::Level::Error;
}

void TestLogMapper::mapsLevel()
{
    QFETCH(QUaLogLevel, input);
    QFETCH(LogEntry::Level, expected);

    const std::optional<LogEntry> entry =
        LogMapper::fromQUaLog(QUaLog(QStringLiteral("m"), input, QUaLogCategory::Server));
    QVERIFY(entry.has_value());
    QCOMPARE(entry->level, expected);
}

void TestLogMapper::dropsTraceAndDebug()
{
    QVERIFY(!LogMapper::fromQUaLog(QUaLog(QStringLiteral("m"), QUaLogLevel::Trace, QUaLogCategory::Server)));
    QVERIFY(!LogMapper::fromQUaLog(QUaLog(QStringLiteral("m"), QUaLogLevel::Debug, QUaLogCategory::Server)));
}

void TestLogMapper::mapsCategory_data()
{
    QTest::addColumn<QUaLogCategory>("input");
    QTest::addColumn<LogEntry::Category>("expected");
    QTest::addColumn<QString>("source");

    QTest::newRow("session") << QUaLogCategory::Session << LogEntry::Category::Session << "Session";
    QTest::newRow("channel") << QUaLogCategory::SecureChannel << LogEntry::Category::Security << "SecureChannel";
    QTest::newRow("policy") << QUaLogCategory::SecurityPolicy << LogEntry::Category::Security << "SecurityPolicy";
    QTest::newRow("network") << QUaLogCategory::Network << LogEntry::Category::Server << "Network";
    QTest::newRow("serialization") << QUaLogCategory::Serialization << LogEntry::Category::Server
                                   << "Serialization";
}

void TestLogMapper::mapsCategory()
{
    QFETCH(QUaLogCategory, input);
    QFETCH(LogEntry::Category, expected);
    QFETCH(QString, source);

    const std::optional<LogEntry> entry = LogMapper::fromQUaLog(QUaLog(QStringLiteral("m"), QUaLogLevel::Info, input));
    QVERIFY(entry.has_value());
    QCOMPARE(entry->category, expected);
    QCOMPARE(entry->source, source);
}

void TestLogMapper::keepsMessageAndTime()
{
    QUaLog log(QByteArray("Connection opened"), QUaLogLevel::Info, QUaLogCategory::Network);
    log.timestamp = QDateTime(QDate(2026, 3, 4), QTime(5, 6, 7));
    const std::optional<LogEntry> entry = LogMapper::fromQUaLog(log);
    QCOMPARE(entry->message, QStringLiteral("Connection opened"));
    QCOMPARE(entry->time, log.timestamp);
}

void TestLogMapper::buildsOwnEntries()
{
    const QDateTime before = QDateTime::currentDateTime();
    const LogEntry entry = LogMapper::entry(LogEntry::Level::Warning, LogEntry::Category::Security, QStringLiteral("x"));
    QCOMPARE(entry.level, LogEntry::Level::Warning);
    QCOMPARE(entry.category, LogEntry::Category::Security);
    QCOMPARE(entry.message, QStringLiteral("x"));
    QCOMPARE(entry.source, QStringLiteral("OpenUaSim"));
    QVERIFY(entry.time >= before);
}

QTEST_GUILESS_MAIN(TestLogMapper)

#include "test_logmapper.moc"
