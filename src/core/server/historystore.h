// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file historystore.h
/// \brief Declares the in-memory value history clients read with HistoryRead.
///

#pragma once

#include <QDateTime>
#include <QHash>
#include <QMap>
#include <QQueue>
#include <QVariant>
#include <QVector>

#include <QUaHistoryBackend>

#include "data/projectconfig.h"

///
/// \brief Keeps the recent values of historizing variables in memory; QUaServer::setHistorizer() takes it.
///
/// The historizer methods follow the contract of QUaHistoryBackend, whose ClosestFromAbove and ClosestFromBelow
/// matches are inverted to select the values inside a read range.
///
class HistoryStore
{
public:
    ///
    /// \brief Changes how much history is kept and drops what the new limits exclude.
    /// \param limits Value count and age; clamped to 1..kMaxHistoryValues and 0..kMaxHistoryAgeMinutes.
    ///
    void setLimits(const HistoryLimits &limits);

    ///
    /// \brief Returns the limits in force.
    /// \return Clamped limits.
    ///
    HistoryLimits limits() const;

    ///
    /// \brief Drops the history of every variable.
    ///
    void clear();

    ///
    /// \brief Records a value and drops the values the limits exclude.
    /// \param nodeId Variable node id.
    /// \param dataPoint Value with its source timestamp and status; replaces a value with the same timestamp.
    /// \param logOut Unused; recording cannot fail.
    /// \return Always true.
    ///
    bool writeHistoryData(const QUaNodeId &nodeId, const QUaHistoryDataPoint &dataPoint, QQueue<QUaLog> &logOut);

    ///
    /// \brief Replaces a recorded value.
    /// \param nodeId Variable node id.
    /// \param dataPoint Value with the timestamp of the value to replace.
    /// \param logOut Unused; recording cannot fail.
    /// \return Always true.
    ///
    bool updateHistoryData(const QUaNodeId &nodeId, const QUaHistoryDataPoint &dataPoint, QQueue<QUaLog> &logOut);

    ///
    /// \brief Drops the values of a time range.
    /// \param nodeId Variable node id.
    /// \param timeStart First timestamp to drop.
    /// \param timeEnd Last timestamp to drop; invalid drops up to the newest value.
    /// \param logOut Unused.
    /// \return False when the variable has no history.
    ///
    bool removeHistoryData(const QUaNodeId &nodeId, const QDateTime &timeStart, const QDateTime &timeEnd,
                           QQueue<QUaLog> &logOut);

    ///
    /// \brief Returns the timestamp of the oldest value.
    /// \param nodeId Variable node id.
    /// \param logOut Unused.
    /// \return Timestamp, or invalid when the variable has no history.
    ///
    QDateTime firstTimestamp(const QUaNodeId &nodeId, QQueue<QUaLog> &logOut) const;

    ///
    /// \brief Returns the timestamp of the newest value.
    /// \param nodeId Variable node id.
    /// \param logOut Unused.
    /// \return Timestamp, or invalid when the variable has no history.
    ///
    QDateTime lastTimestamp(const QUaNodeId &nodeId, QQueue<QUaLog> &logOut) const;

    ///
    /// \brief Tells whether a value has exactly a timestamp.
    /// \param nodeId Variable node id.
    /// \param timestamp Timestamp to look for.
    /// \param logOut Unused.
    /// \return True when a value has it.
    ///
    bool hasTimestamp(const QUaNodeId &nodeId, const QDateTime &timestamp, QQueue<QUaLog> &logOut) const;

    ///
    /// \brief Finds the timestamp next to one that no value has.
    /// \param nodeId Variable node id.
    /// \param timestamp Timestamp to start from.
    /// \param match ClosestFromAbove for the first later value, ClosestFromBelow for the last earlier one.
    /// \param logOut Unused.
    /// \return Timestamp found, the newest or oldest one when there is none in that direction, or invalid when
    ///         the variable has no history.
    ///
    QDateTime findTimestamp(const QUaNodeId &nodeId, const QDateTime &timestamp,
                            const QUaHistoryBackend::TimeMatch &match, QQueue<QUaLog> &logOut) const;

    ///
    /// \brief Counts the values of a time range.
    /// \param nodeId Variable node id.
    /// \param timeStart First timestamp of the range.
    /// \param timeEnd Last timestamp of the range; invalid counts up to the newest value.
    /// \param logOut Unused.
    /// \return Number of values, including those at both ends.
    ///
    quint64 numDataPointsInRange(const QUaNodeId &nodeId, const QDateTime &timeStart, const QDateTime &timeEnd,
                                 QQueue<QUaLog> &logOut) const;

    ///
    /// \brief Reads a page of values in time order.
    /// \param nodeId Variable node id.
    /// \param timeStart Timestamp the page counts from.
    /// \param numPointsOffset Values to skip after timeStart, for continuation points.
    /// \param numPointsToRead Page size.
    /// \param logOut Unused.
    /// \return Exactly numPointsToRead points; those past the newest value have an invalid timestamp.
    ///
    QVector<QUaHistoryDataPoint> readHistoryData(const QUaNodeId &nodeId, const QDateTime &timeStart,
                                                 const quint64 &numPointsOffset, const quint64 &numPointsToRead,
                                                 QQueue<QUaLog> &logOut) const;

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    ///
    /// \brief Refuses to record an event; the server keeps no event history.
    /// \return Always false.
    ///
    bool writeHistoryEventsOfType(const QUaNodeId &eventTypeNodeId, const QList<QUaNodeId> &emittersNodeIds,
                                  const QUaHistoryEventPoint &eventPoint, QQueue<QUaLog> &logOut);

    ///
    /// \brief Lists the event types recorded for an emitter.
    /// \return Always empty.
    ///
    QVector<QUaNodeId> eventTypesOfEmitter(const QUaNodeId &emitterNodeId, QQueue<QUaLog> &logOut);

    ///
    /// \brief Finds the timestamp of a recorded event.
    /// \return Always invalid.
    ///
    QDateTime findTimestampEventOfType(const QUaNodeId &emitterNodeId, const QUaNodeId &eventTypeNodeId,
                                       const QDateTime &timestamp, const QUaHistoryBackend::TimeMatch &match,
                                       QQueue<QUaLog> &logOut);

    ///
    /// \brief Counts the recorded events of a time range.
    /// \return Always 0.
    ///
    quint64 numEventsOfTypeInRange(const QUaNodeId &emitterNodeId, const QUaNodeId &eventTypeNodeId,
                                   const QDateTime &timeStart, const QDateTime &timeEnd, QQueue<QUaLog> &logOut);

    ///
    /// \brief Reads a page of recorded events.
    /// \return Always empty.
    ///
    QVector<QUaHistoryEventPoint> readHistoryEventsOfType(const QUaNodeId &emitterNodeId,
                                                          const QUaNodeId &eventTypeNodeId,
                                                          const QDateTime &timeStart, const quint64 &numPointsOffset,
                                                          const quint64 &numPointsToRead,
                                                          const QList<QUaBrowsePath> &columnsToRead,
                                                          QQueue<QUaLog> &logOut);
#endif

private:
    struct Sample
    {
        QVariant value;
        quint32 status = 0;
    };
    using Table = QMap<QDateTime, Sample>;

    const Table *table(const QUaNodeId &nodeId) const;
    void trim(Table &table) const;

    QHash<QUaNodeId, Table> _tables;
    HistoryLimits _limits;
};
