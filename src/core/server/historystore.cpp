// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file historystore.cpp
/// \brief Implements the in-memory value history clients read with HistoryRead.
///

#include <iterator>

#include "historystore.h"

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
}

///
/// \brief Records a value and drops the values the limits exclude.
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
    table.insert(dataPoint.timestamp, { dataPoint.value, dataPoint.status });
    trim(table);
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
    const auto first = table.lowerBound(timeStart);
    const auto last = timeEnd.isValid() ? table.upperBound(timeEnd) : table.end();
    if (!timeEnd.isValid() || timeStart <= timeEnd)
        table.erase(first, last);
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
    return values ? values->firstKey() : QDateTime();
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
    return values ? values->lastKey() : QDateTime();
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
    return values && values->contains(timestamp);
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
        const auto later = values->upperBound(timestamp);
        return later == values->cend() ? values->lastKey() : later.key();
    }
    const auto notEarlier = values->lowerBound(timestamp);
    return notEarlier == values->cbegin() ? values->firstKey() : std::prev(notEarlier).key();
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
    const auto last = timeEnd.isValid() ? values->upperBound(timeEnd) : values->cend();
    return quint64(std::distance(values->lowerBound(timeStart), last));
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
    auto it = values->lowerBound(timeStart);
    for (quint64 skipped = 0; skipped < numPointsOffset && it != values->cend(); ++skipped)
        ++it;
    for (QUaHistoryDataPoint &point : points) {
        if (it == values->cend())
            break;
        point = { it.key(), it->value, it->status };
        ++it;
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
/// \brief Drops the oldest values beyond the count limit and those older than the age limit.
/// \param table Values of one variable.
///
void HistoryStore::trim(Table &table) const
{
    while (table.size() > _limits.maxValues)
        table.erase(table.begin());
    if (_limits.maxAgeMinutes > 0 && !table.isEmpty()) {
        const QDateTime oldest = table.lastKey().addSecs(-qint64(_limits.maxAgeMinutes) * 60);
        table.erase(table.begin(), table.lowerBound(oldest));
    }
}
