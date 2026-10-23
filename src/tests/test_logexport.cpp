// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include "formatting.h"
#include "logexport.h"

namespace {

LogEntry entry(const QString &message)
{
    LogEntry result;
    result.time = QDateTime(QDate(2026, 10, 19), QTime(14, 31, 20, 4));
    result.level = LogEntry::Level::Warning;
    result.category = LogEntry::Category::Session;
    result.message = message;
    result.source = QStringLiteral("Server");
    return result;
}

QStringList records(const QString &csv)
{
    QStringList lines = csv.split(QStringLiteral("\r\n"));
    if (!lines.isEmpty() && lines.constLast().isEmpty())
        lines.removeLast();
    return lines;
}

}

class TestLogExport : public QObject
{
    Q_OBJECT

private slots:
    void writesHeaderForEmptyLog();
    void writesEntryFields();
    void quotesSpecialCharacters();
    void writesUtf8FileWithByteOrderMark();
    void reportsUnwritablePath();

private:
    QTemporaryDir _directory;
};

void TestLogExport::writesHeaderForEmptyLog()
{
    QCOMPARE(records(LogExport::toCsv({})),
             QStringList { QStringLiteral("Time,Level,Category,Source,Message") });
}

void TestLogExport::writesEntryFields()
{
    const LogEntry logged = entry(QStringLiteral("Session opened"));
    const QStringList lines = records(LogExport::toCsv({ logged }));
    QCOMPARE(lines.size(), 2);
    QCOMPARE(lines.at(1), QStringLiteral("%1,WARNING,Session,Server,Session opened")
                              .arg(Formatting::preciseDateTime(logged.time)));
}

void TestLogExport::quotesSpecialCharacters()
{
    const QString csv = LogExport::toCsv({ entry(QStringLiteral("a, \"b\"\nc")) });
    QVERIFY(csv.endsWith(QStringLiteral(",Server,\"a, \"\"b\"\"\nc\"\r\n")));
}

void TestLogExport::writesUtf8FileWithByteOrderMark()
{
    const QString fileName = _directory.filePath(QStringLiteral("log.csv"));
    QVERIFY(LogExport::write(fileName, { entry(QStringLiteral("Температура")) }));
    QFile file(fileName);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray content = file.readAll();
    QVERIFY(content.startsWith("\xEF\xBB\xBF"));
    QVERIFY(QString::fromUtf8(content).contains(QStringLiteral("Температура")));
}

void TestLogExport::reportsUnwritablePath()
{
    QString error;
    QVERIFY(!LogExport::write(_directory.filePath(QStringLiteral("no/such/dir/log.csv")), {}, &error));
    QVERIFY(!error.isEmpty());
}

QTEST_GUILESS_MAIN(TestLogExport)

#include "test_logexport.moc"
