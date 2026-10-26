// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file historystore.cpp
/// \brief Implements the in-memory value history clients read with HistoryRead.
///

#include <algorithm>

#include <QSequentialIterable>

#include "historystore.h"
#include "simulation/valuecast.h"

namespace {

///
/// \brief Returns how much of the history budget a value takes.
/// \param value Recorded value.
/// \return Length of an array value, at least 1; 1 for a scalar.
///
qsizetype costOf(const QVariant &value)
{
    if (!ValueCast::isArray(value))
        return 1;
    return std::max<qsizetype>(1, value.value<QSequentialIterable>().size());
}

}

///
/// \brief Creates an empty history.
/// \param budget Values kept across all variables, an array value counting as its length; clamped to at
///        least 1.
///
HistoryStore::HistoryStore(qint64 budget)
    : _budget(std::max<qint64>(1, budget))
{
}

///
/// \brief Changes how much history is kept and drops what the new limits exclude.
/// \param limits Value count and age; clamped to 1..kMaxHistoryValues and 0..kMaxHistoryAgeMinutes.
///
void HistoryStore::setLimits(const HistoryLimits &limits)
{
    _limits.maxValues = qBound(1, limits.maxValues, kMaxHistoryValues);
    _limits.maxAgeMinutes = qBound(0, limits.maxAgeMinutes, kMaxHistoryAgeMinutes);
    for (Table &table : _tables)
        trim(table);
}

///
/// \brief Returns the limits in force.
/// \return Clamped limits.
///
HistoryLimits HistoryStore::limits() const
{
    return _limits;
}

///
/// \brief Drops the history of every variable.
///
void HistoryStore::clear()
{
    _tables.clear();
    _cost = 0;
}

///
/// \brief Records a value and drops the values the limits exclude, then the oldest values of any variable
///        while the budget is exceeded; the recorded value itself is kept.
/// \param nodeId Variable node id.
/// \param dataPoint Value with its source timestamp and status; replaces a value with the same timestamp.
/// \param logOut Unused; recording cannot fail.
/// \return Always true.
///
bool HistoryStore::writeHistoryData(const QUaNodeId &nodeId, const QUaHistoryDataPoint &dataPoint,
                                    QQueue<QUaLog> &logOut)
{
    Q_UNUSED(logOut);
    Table &table = _tables[nodeId];
    Sample sample { dataPoint.timestamp, dataPoint.value, dataPoint.status, costOf(dataPoint.value) };
    _cost += sample.cost;
    const qsizetype index = table.isEmpty() || table.constLast().timestamp < sample.timestamp
        ? table.size()
        : lowerIndex(table, sample.timestamp);
    if (index < table.size() && table.at(index).timestamp == sample.timestamp) {
        _cost -= table.at(index).cost;
        table[index] = std::move(sample);
    } else {
        table.insert(index, std::move(sample));
    }
    trim(table);
    enforceBudget(nodeId);
    return true;
}

///
/// \brief Replaces a recorded value.
/// \param nodeId Variable node id.
/// \param dataPoint Value with the timestamp of the value to replace.
/// \param logOut Unused; recording cannot fail.
/// \return Always true.
///
bool HistoryStore::updateHistoryData(const QUaNodeId &nodeId, const QUaHistoryDataPoint &dataPoint,
                                     QQueue<QUaLog> &logOut)
{
    return writeHistoryData(nodeId, dataPoint, logOut);
}

///
/// \brief Drops the values of a time range.
/// \param nodeId Variable node id.
/// \param timeStart First timestamp to drop.
/// \param timeEnd Last timestamp to drop; invalid drops up to the newest value.
/// \param logOut Unused.
/// \return False when the variable has no history.
///
bool HistoryStore::removeHistoryData(const QUaNodeId &nodeId, const QDateTime &timeStart, const QDateTime &timeEnd,
                                     QQueue<QUaLog> &logOut)
{
    Q_UNUSED(logOut);
    const auto found = _tables.find(nodeId);
    if (found == _tables.end())
        return false;
    Table &table = *found;
    if (!timeEnd.isValid() || timeStart <= timeEnd)
        erase(table, lowerIndex(table, timeStart), timeEnd.isValid() ? upperIndex(table, timeEnd) : table.size());
    return true;
}

///
/// \brief Returns the timestamp of the oldest value.
/// \param nodeId Variable node id.
/// \param logOut Unused.
/// \return Timestamp, or invalid when the variable has no history.
///
QDateTime HistoryStore::firstTimestamp(const QUaNodeId &nodeId, QQueue<QUaLog> &logOut) const
{
    Q_UNUSED(logOut);
    const Table *values = table(nodeId);
    return values ? values->constFirst().timestamp : QDateTime();
}

///
/// \brief Returns the timestamp of the newest value.
/// \param nodeId Variable node id.
/// \param logOut Unused.
/// \return Timestamp, or invalid when the variable has no history.
///
QDateTime HistoryStore::lastTimestamp(const QUaNodeId &nodeId, QQueue<QUaLog> &logOut) const
{
    Q_UNUSED(logOut);
    const Table *values = table(nodeId);
    return values ? values->constLast().timestamp : QDateTime();
}

///
/// \brief Tells whether a value has exactly a timestamp.
/// \param nodeId Variable node id.
/// \param timestamp Timestamp to look for.
/// \param logOut Unused.
/// \return True when a value has it.
///
bool HistoryStore::hasTimestamp(const QUaNodeId &nodeId, const QDateTime &timestamp, QQueue<QUaLog> &logOut) const
{
    Q_UNUSED(logOut);
    const Table *values = table(nodeId);
    if (!values)
        return false;
    const qsizetype index = lowerIndex(*values, timestamp);
    return index < values->size() && values->at(index).timestamp == timestamp;
}

///
/// \brief Finds the timestamp next to one that no value has.
/// \param nodeId Variable node id.
/// \param timestamp Timestamp to start from.
/// \param match ClosestFromAbove for the first later value, ClosestFromBelow for the last earlier one.
/// \param logOut Unused.
/// \return Timestamp found, the newest or oldest one when there is none in that direction, or invalid when
///         the variable has no history.
///
QDateTime HistoryStore::findTimestamp(const QUaNodeId &nodeId, const QDateTime &timestamp,
                                      const QUaHistoryBackend::TimeMatch &match, QQueue<QUaLog> &logOut) const
{
    Q_UNUSED(logOut);
    const Table *values = table(nodeId);
    if (!values)
        return {};
    if (match == QUaHistoryBackend::TimeMatch::ClosestFromAbove) {
        const qsizetype later = upperIndex(*values, timestamp);
        return later == values->size() ? values->constLast().timestamp : values->at(later).timestamp;
    }
    const qsizetype notEarlier = lowerIndex(*values, timestamp);
    return notEarlier == 0 ? values->constFirst().timestamp : values->at(notEarlier - 1).timestamp;
}

///
/// \brief Counts the values of a time range.
/// \param nodeId Variable node id.
/// \param timeStart First timestamp of the range.
/// \param timeEnd Last timestamp of the range; invalid counts up to the newest value.
/// \param logOut Unused.
/// \return Number of values, including those at both ends.
///
quint64 HistoryStore::numDataPointsInRange(const QUaNodeId &nodeId, const QDateTime &timeStart,
                                           const QDateTime &timeEnd, QQueue<QUaLog> &logOut) const
{
    Q_UNUSED(logOut);
    const Table *values = table(nodeId);
    if (!values || (timeEnd.isValid() && timeEnd < timeStart))
        return 0;
    const qsizetype last = timeEnd.isValid() ? upperIndex(*values, timeEnd) : values->size();
    return quint64(last - lowerIndex(*values, timeStart));
}

///
/// \brief Reads a page of values in time order.
/// \param nodeId Variable node id.
/// \param timeStart Timestamp the page counts from.
/// \param numPointsOffset Values to skip after timeStart, for continuation points.
/// \param numPointsToRead Page size.
/// \param logOut Unused.
/// \return Exactly numPointsToRead points; those past the newest value have an invalid timestamp.
///
QVector<QUaHistoryDataPoint> HistoryStore::readHistoryData(const QUaNodeId &nodeId, const QDateTime &timeStart,
                                                           const quint64 &numPointsOffset,
                                                           const quint64 &numPointsToRead,
                                                           QQueue<QUaLog> &logOut) const
{
    Q_UNUSED(logOut);
    // QUaHistoryBackend refuses a page of another size, so missing values are padded.
    QVector<QUaHistoryDataPoint> points(qsizetype(numPointsToRead), QUaHistoryDataPoint { {}, {}, 0 });
    const Table *values = table(nodeId);
    if (!values)
        return points;
    const quint64 start = quint64(lowerIndex(*values, timeStart)) + numPointsOffset;
    const quint64 available = start < quint64(values->size()) ? quint64(values->size()) - start : 0;
    const qsizetype count = qsizetype(std::min(available, numPointsToRead));
    for (qsizetype i = 0; i < count; ++i) {
        const Sample &sample = values->at(qsizetype(start) + i);
        points[i] = { sample.timestamp, sample.value, sample.status };
    }
    return points;
}

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
///
/// \brief Refuses to record an event; the server keeps no event history.
/// \return Always false.
///
bool HistoryStore::writeHistoryEventsOfType(const QUaNodeId &, const QList<QUaNodeId> &,
                                            const QUaHistoryEventPoint &, QQueue<QUaLog> &)
{
    return false;
}

///
/// \brief Lists the event types recorded for an emitter.
/// \return Always empty.
///
QVector<QUaNodeId> HistoryStore::eventTypesOfEmitter(const QUaNodeId &, QQueue<QUaLog> &)
{
    return {};
}

///
/// \brief Finds the timestamp of a recorded event.
/// \return Always invalid.
///
QDateTime HistoryStore::findTimestampEventOfType(const QUaNodeId &, const QUaNodeId &, const QDateTime &,
                                                 const QUaHistoryBackend::TimeMatch &, QQueue<QUaLog> &)
{
    return {};
}

///
/// \brief Counts the recorded events of a time range.
/// \return Always 0.
///
quint64 HistoryStore::numEventsOfTypeInRange(const QUaNodeId &, const QUaNodeId &, const QDateTime &,
                                             const QDateTime &, QQueue<QUaLog> &)
{
    return 0;
}

///
/// \brief Reads a page of recorded events.
/// \return Always empty.
///
QVector<QUaHistoryEventPoint> HistoryStore::readHistoryEventsOfType(const QUaNodeId &, const QUaNodeId &,
                                                                    const QDateTime &, const quint64 &,
                                                                    const quint64 &, const QList<QUaBrowsePath> &,
                                                                    QQueue<QUaLog> &)
{
    return {};
}
#endif

///
/// \brief Returns the values of a variable.
/// \param nodeId Variable node id.
/// \return Values, or null when the variable has none.
///
const HistoryStore::Table *HistoryStore::table(const QUaNodeId &nodeId) const
{
    const auto found = _tables.constFind(nodeId);
    return found == _tables.cend() || found->isEmpty() ? nullptr : &*found;
}

///
/// \brief Finds the first value not earlier than a timestamp.
/// \param table Values of one variable.
/// \param timestamp Timestamp to look for.
/// \return Index of the value, or the table size when every value is earlier.
///
qsizetype HistoryStore::lowerIndex(const Table &table, const QDateTime &timestamp)
{
    const auto found = std::lower_bound(table.cbegin(), table.cend(), timestamp,
                                        [](const Sample &sample, const QDateTime &time) {
                                            return sample.timestamp < time;
                                        });
    return found - table.cbegin();
}

///
/// \brief Finds the first value later than a timestamp.
/// \param table Values of one variable.
/// \param timestamp Timestamp to look for.
/// \return Index of the value, or the table size when no value is later.
///
qsizetype HistoryStore::upperIndex(const Table &table, const QDateTime &timestamp)
{
    const auto found = std::upper_bound(table.cbegin(), table.cend(), timestamp,
                                        [](const QDateTime &time, const Sample &sample) {
                                            return time < sample.timestamp;
                                        });
    return found - table.cbegin();
}

///
/// \brief Drops a range of values and releases their share of the budget.
/// \param table Values of one variable.
/// \param first Index of the first value to drop.
/// \param last Index past the last value to drop.
///
void HistoryStore::erase(Table &table, qsizetype first, qsizetype last)
{
    for (qsizetype i = first; i < last; ++i)
        _cost -= table.at(i).cost;
    table.remove(first, last - first);
}

///
/// \brief Drops the oldest values beyond the count limit and those older than the age limit.
/// \param table Values of one variable.
///
void HistoryStore::trim(Table &table)
{
    if (table.size() > _limits.maxValues)
        erase(table, 0, table.size() - _limits.maxValues);
    if (_limits.maxAgeMinutes > 0 && !table.isEmpty()) {
        const QDateTime oldest = table.constLast().timestamp.addSecs(-qint64(_limits.maxAgeMinutes) * 60);
        erase(table, 0, lowerIndex(table, oldest));
    }
}

///
/// \brief Drops the oldest value of any variable until the history fits its budget, never the newest value of
///        the variable just written.
/// \param written Node id of the variable just written.
///
void HistoryStore::enforceBudget(const QUaNodeId &written)
{
    while (_cost > _budget) {
        auto oldest = _tables.end();
        for (auto it = _tables.begin(); it != _tables.end(); ++it) {
            const qsizetype keep = it.key() == written ? 1 : 0;
            if (it->size() <= keep)
                continue;
            if (oldest == _tables.end() || it->constFirst().timestamp < oldest->constFirst().timestamp)
                oldest = it;
        }
        if (oldest == _tables.end())
            return;
        erase(*oldest, 0, 1);
        if (oldest->isEmpty())
            _tables.erase(oldest);
    }
}
