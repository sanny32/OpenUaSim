// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QTest>

#include "server/historystore.h"

namespace {

const QUaNodeId kLevel(QStringLiteral("ns=1;s=Tank.Level"));
const QUaNodeId kPressure(QStringLiteral("ns=1;s=Tank.Pressure"));
const QDateTime kStart = QDateTime(QDate(2026, 1, 1), QTime(12, 0), QTimeZone::UTC);

QDateTime at(int seconds)
{
    return kStart.addSecs(seconds);
}

void write(HistoryStore &store, const QUaNodeId &nodeId, int seconds, const QVariant &value, quint32 status = 0)
{
    QQueue<QUaLog> log;
    QVERIFY(store.writeHistoryData(nodeId, { at(seconds), value, status }, log));
}

void write(HistoryStore &store, int seconds, const QVariant &value, quint32 status = 0)
{
    write(store, kLevel, seconds, value, status);
}

QVariantList values(const HistoryStore &store, const QUaNodeId &nodeId = kLevel)
{
    QQueue<QUaLog> log;
    const QDateTime first = store.firstTimestamp(nodeId, log);
    if (!first.isValid())
        return {};
    const quint64 count = store.numDataPointsInRange(nodeId, first, QDateTime(), log);
    QVariantList result;
    for (const QUaHistoryDataPoint &point : store.readHistoryData(nodeId, first, 0, count, log))
        result.append(point.value);
    return result;
}

}

class TestHistoryStore : public QObject
{
    Q_OBJECT

private slots:
    void recordsValuesInTimeOrder();
    void insertsValueBetweenRecordedOnes();
    void replacesValueWithSameTimestamp();
    void dropsOldestBeyondCount();
    void dropsValuesOlderThanAge();
    void setLimitsClampsAndTrims();
    void unknownVariableHasNoHistory();
    void findsNeighbouringTimestamps();
    void countsRange();
    void readsPagesAndPadsMissingValues();
    void removesRange();
    void clearDropsEverything();
    void budgetDropsOldestValueOfAnyVariable();
    void budgetCountsArrayLength();
    void budgetKeepsWrittenValue();
    void replacedValueReleasesBudget();
    void removedValuesReleaseBudget();
    void clearReleasesBudget();
};

void TestHistoryStore::recordsValuesInTimeOrder()
{
    HistoryStore store;
    write(store, 2, 20.0);
    write(store, 1, 10.0, 0x80000000);
    write(store, 3, 30.0);

    QQueue<QUaLog> log;
    QCOMPARE(store.firstTimestamp(kLevel, log), at(1));
    QCOMPARE(store.lastTimestamp(kLevel, log), at(3));
    QCOMPARE(values(store), QVariantList({ 10.0, 20.0, 30.0 }));
    const QVector<QUaHistoryDataPoint> first = store.readHistoryData(kLevel, at(1), 0, 1, log);
    QCOMPARE(first.first().timestamp, at(1));
    QCOMPARE(first.first().status, 0x80000000u);
    QVERIFY(log.isEmpty());
}

void TestHistoryStore::insertsValueBetweenRecordedOnes()
{
    HistoryStore store;
    write(store, 1, 10.0);
    write(store, 3, 30.0);
    write(store, 2, 20.0);
    write(store, 3, 31.0);
    QCOMPARE(values(store), QVariantList({ 10.0, 20.0, 31.0 }));
}

void TestHistoryStore::replacesValueWithSameTimestamp()
{
    HistoryStore store;
    write(store, 1, 10.0);
    QQueue<QUaLog> log;
    QVERIFY(store.updateHistoryData(kLevel, { at(1), 11.0, 0 }, log));
    QCOMPARE(values(store), QVariantList({ QVariant(11.0) }));
}

void TestHistoryStore::dropsOldestBeyondCount()
{
    HistoryStore store;
    store.setLimits({ 3, 0 });
    for (int second = 1; second <= 5; ++second)
        write(store, second, second * 10.0);
    QCOMPARE(values(store), QVariantList({ 30.0, 40.0, 50.0 }));
}

void TestHistoryStore::dropsValuesOlderThanAge()
{
    HistoryStore store;
    store.setLimits({ 100, 2 });
    write(store, 0, 1.0);
    write(store, 60, 2.0);
    write(store, 120, 3.0);
    QCOMPARE(values(store), QVariantList({ 1.0, 2.0, 3.0 }));

    write(store, 121, 4.0);
    QCOMPARE(values(store), QVariantList({ 2.0, 3.0, 4.0 }));
}

void TestHistoryStore::setLimitsClampsAndTrims()
{
    HistoryStore store;
    for (int second = 1; second <= 5; ++second)
        write(store, second, second * 10.0);

    store.setLimits({ 0, -5 });
    QCOMPARE(store.limits().maxValues, 1);
    QCOMPARE(store.limits().maxAgeMinutes, 0);
    QCOMPARE(values(store), QVariantList({ QVariant(50.0) }));

    store.setLimits({ kMaxHistoryValues + 1, kMaxHistoryAgeMinutes + 1 });
    QCOMPARE(store.limits().maxValues, kMaxHistoryValues);
    QCOMPARE(store.limits().maxAgeMinutes, kMaxHistoryAgeMinutes);
}

void TestHistoryStore::unknownVariableHasNoHistory()
{
    HistoryStore store;
    QQueue<QUaLog> log;
    QVERIFY(!store.firstTimestamp(kLevel, log).isValid());
    QVERIFY(!store.lastTimestamp(kLevel, log).isValid());
    QVERIFY(!store.hasTimestamp(kLevel, at(0), log));
    QVERIFY(!store.findTimestamp(kLevel, at(0), QUaHistoryBackend::TimeMatch::ClosestFromAbove, log).isValid());
    QCOMPARE(store.numDataPointsInRange(kLevel, at(0), at(10), log), 0u);
    QVERIFY(!store.removeHistoryData(kLevel, at(0), at(10), log));
    const QVector<QUaHistoryDataPoint> points = store.readHistoryData(kLevel, at(0), 0, 2, log);
    QCOMPARE(points.size(), 2);
    QVERIFY(!points.first().timestamp.isValid());
    QVERIFY(log.isEmpty());
}

void TestHistoryStore::findsNeighbouringTimestamps()
{
    HistoryStore store;
    write(store, 10, 1.0);
    write(store, 20, 2.0);
    write(store, 30, 3.0);
    QQueue<QUaLog> log;
    const auto above = QUaHistoryBackend::TimeMatch::ClosestFromAbove;
    const auto below = QUaHistoryBackend::TimeMatch::ClosestFromBelow;

    QVERIFY(store.hasTimestamp(kLevel, at(20), log));
    QVERIFY(!store.hasTimestamp(kLevel, at(25), log));
    QCOMPARE(store.findTimestamp(kLevel, at(15), above, log), at(20));
    QCOMPARE(store.findTimestamp(kLevel, at(20), above, log), at(30));
    QCOMPARE(store.findTimestamp(kLevel, at(35), above, log), at(30));
    QCOMPARE(store.findTimestamp(kLevel, at(25), below, log), at(20));
    QCOMPARE(store.findTimestamp(kLevel, at(20), below, log), at(10));
    QCOMPARE(store.findTimestamp(kLevel, at(5), below, log), at(10));
}

void TestHistoryStore::countsRange()
{
    HistoryStore store;
    for (int second = 1; second <= 5; ++second)
        write(store, second, second * 10.0);
    QQueue<QUaLog> log;
    QCOMPARE(store.numDataPointsInRange(kLevel, at(2), at(4), log), 3u);
    QCOMPARE(store.numDataPointsInRange(kLevel, at(2), QDateTime(), log), 4u);
    QCOMPARE(store.numDataPointsInRange(kLevel, at(4), at(2), log), 0u);
}

void TestHistoryStore::readsPagesAndPadsMissingValues()
{
    HistoryStore store;
    for (int second = 1; second <= 5; ++second)
        write(store, second, second * 10.0);
    QQueue<QUaLog> log;

    QVector<QUaHistoryDataPoint> page = store.readHistoryData(kLevel, at(1), 2, 2, log);
    QCOMPARE(page.size(), 2);
    QCOMPARE(page.at(0).value, QVariant(30.0));
    QCOMPARE(page.at(1).value, QVariant(40.0));

    page = store.readHistoryData(kLevel, at(1), 4, 3, log);
    QCOMPARE(page.size(), 3);
    QCOMPARE(page.at(0).value, QVariant(50.0));
    QVERIFY(!page.at(1).timestamp.isValid());
    QVERIFY(!page.at(2).timestamp.isValid());

    // A start trimmed away since the previous page reads on from the oldest kept value.
    page = store.readHistoryData(kLevel, at(0), 0, 1, log);
    QCOMPARE(page.first().value, QVariant(10.0));
}

void TestHistoryStore::removesRange()
{
    HistoryStore store;
    for (int second = 1; second <= 5; ++second)
        write(store, second, second * 10.0);
    QQueue<QUaLog> log;

    QVERIFY(store.removeHistoryData(kLevel, at(2), at(3), log));
    QCOMPARE(values(store), QVariantList({ 10.0, 40.0, 50.0 }));
    QVERIFY(store.removeHistoryData(kLevel, at(5), at(1), log));
    QCOMPARE(values(store), QVariantList({ 10.0, 40.0, 50.0 }));
    QVERIFY(store.removeHistoryData(kLevel, at(4), QDateTime(), log));
    QCOMPARE(values(store), QVariantList({ QVariant(10.0) }));
}

void TestHistoryStore::clearDropsEverything()
{
    HistoryStore store;
    write(store, 1, 10.0);
    store.clear();
    QQueue<QUaLog> log;
    QVERIFY(!store.firstTimestamp(kLevel, log).isValid());
}

void TestHistoryStore::budgetDropsOldestValueOfAnyVariable()
{
    HistoryStore store(4);
    write(store, kLevel, 1, 1.0);
    write(store, kLevel, 2, 2.0);
    write(store, kPressure, 3, 3.0);
    write(store, kPressure, 4, 4.0);
    write(store, kLevel, 5, 5.0);
    QCOMPARE(values(store, kLevel), QVariantList({ 2.0, 5.0 }));
    QCOMPARE(values(store, kPressure), QVariantList({ 3.0, 4.0 }));

    write(store, kPressure, 6, 6.0);
    QCOMPARE(values(store, kLevel), QVariantList({ QVariant(5.0) }));
    QCOMPARE(values(store, kPressure), QVariantList({ 3.0, 4.0, 6.0 }));
}

void TestHistoryStore::budgetCountsArrayLength()
{
    HistoryStore store(5);
    write(store, 1, QVariantList({ 1.0, 2.0, 3.0 }));
    write(store, 2, QVariant::fromValue(QList<double>({ 4.0, 5.0, 6.0 })));
    QQueue<QUaLog> log;
    QCOMPARE(store.firstTimestamp(kLevel, log), at(2));
    QCOMPARE(store.numDataPointsInRange(kLevel, at(0), QDateTime(), log), 1u);

    write(store, 3, QStringLiteral("text counts once"));
    write(store, 4, QByteArray("bytes"));
    QCOMPARE(store.numDataPointsInRange(kLevel, at(0), QDateTime(), log), 3u);
}

void TestHistoryStore::budgetKeepsWrittenValue()
{
    HistoryStore store(2);
    write(store, kLevel, 1, QVariantList({ 1.0, 2.0, 3.0, 4.0, 5.0 }));
    QCOMPARE(values(store, kLevel).size(), 1);

    write(store, kPressure, 2, 2.0);
    QVERIFY(values(store, kLevel).isEmpty());
    QCOMPARE(values(store, kPressure), QVariantList({ QVariant(2.0) }));
}

void TestHistoryStore::replacedValueReleasesBudget()
{
    HistoryStore store(2);
    write(store, 1, QVariantList({ 1.0, 2.0 }));
    write(store, 1, 10.0);
    write(store, 2, 20.0);
    QCOMPARE(values(store), QVariantList({ 10.0, 20.0 }));
}

void TestHistoryStore::removedValuesReleaseBudget()
{
    HistoryStore store(3);
    for (int second = 1; second <= 3; ++second)
        write(store, second, second * 10.0);
    QQueue<QUaLog> log;
    QVERIFY(store.removeHistoryData(kLevel, at(1), at(2), log));
    write(store, 4, 40.0);
    write(store, 5, 50.0);
    QCOMPARE(values(store), QVariantList({ 30.0, 40.0, 50.0 }));
}

void TestHistoryStore::clearReleasesBudget()
{
    HistoryStore store(2);
    write(store, 1, 10.0);
    write(store, 2, 20.0);
    store.clear();
    write(store, 3, 30.0);
    write(store, 4, 40.0);
    QCOMPARE(values(store), QVariantList({ 30.0, 40.0 }));
}

QTEST_GUILESS_MAIN(TestHistoryStore)
#include "test_historystore.moc"
