// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file simulationengine.h
/// \brief Declares the timer-driven generation of simulated variable values.
///

#pragma once

#include <functional>

#include <QElapsedTimer>
#include <QHash>
#include <QList>
#include <QObject>
#include <QRandomGenerator>

#include "data/projectconfig.h"

class QTimer;
class ValueSink;

///
/// \brief A variable the engine drives.
///
struct SimulatedVariable
{
    /// \brief Variable node id.
    QString nodeId;
    /// \brief Built-in data type name the values are converted to.
    QString dataType;
    /// \brief Waveform, interval, period, amplitude and offset.
    SimulationSettings settings;
    /// \brief Number of array elements, each simulated with its own phase; 0 for a scalar.
    int arrayLength = 0;
};

///
/// \brief Writes waveform values to variables, one timer per distinct update interval.
///
class SimulationEngine : public QObject
{
    Q_OBJECT

public:
    /// \brief Shortest update interval; faster settings are clamped to it.
    static constexpr int kMinimumIntervalMs = 10;

    ///
    /// \brief Creates a stopped engine.
    /// \param sink Receiver of the values; must outlive the engine.
    /// \param parent Owning QObject.
    ///
    explicit SimulationEngine(ValueSink *sink, QObject *parent = nullptr);

    ///
    /// \brief Collects the simulated variables of a node tree.
    /// \param nodes Project node tree.
    /// \return Variables whose function is not None.
    ///
    static QList<SimulatedVariable> variablesOf(const QList<NodeDefinition> &nodes);

    ///
    /// \brief Replaces the driven variables.
    /// \param variables Variables to drive; ones with function None are ignored.
    ///
    void setVariables(const QList<SimulatedVariable> &variables);

    ///
    /// \brief Adds, changes or, for function None, stops driving one variable.
    /// \param variable New settings of the variable.
    ///
    void updateVariable(const SimulatedVariable &variable);

    ///
    /// \brief Stops driving a variable.
    /// \param nodeId Variable node id.
    ///
    void removeVariable(const QString &nodeId);

    ///
    /// \brief Returns the node ids of the driven variables.
    /// \return Node ids in no particular order.
    ///
    QStringList variableIds() const;

    ///
    /// \brief Returns the update intervals that have a timer.
    /// \return Clamped intervals in ascending order.
    ///
    QList<int> intervals() const;

    ///
    /// \brief Restarts the waveforms at time zero and starts the timers.
    ///
    void start();

    ///
    /// \brief Stops the timers; values keep their last state.
    ///
    void stop();

    ///
    /// \brief Tells whether the timers run.
    /// \return True between start() and stop().
    ///
    bool isRunning() const;

    ///
    /// \brief Replaces the time source, so tests control the waveform position.
    /// \param elapsedMs Returns the milliseconds since start(); an empty function restores the steady clock.
    ///
    void setElapsedSource(const std::function<qint64()> &elapsedMs);

    ///
    /// \brief Reseeds the noise of the Random function.
    /// \param seed Seed value.
    ///
    void setRandomSeed(quint32 seed);

    ///
    /// \brief Computes and writes the values of every variable updated at an interval.
    /// \param intervalMs Clamped update interval.
    ///
    void tick(int intervalMs);

signals:
    ///
    /// \brief Announces the values written by one tick.
    /// \param values New values keyed by node id.
    ///
    void valuesUpdated(const QHash<QString, QVariant> &values);

private:
    static int clampedInterval(const SimulationSettings &settings);
    qint64 elapsedMs() const;
    QVariant sample(const SimulatedVariable &variable, qint64 elapsed);
    void rebuildTimers();
    void handleTimeout();

    ValueSink *_sink;
    QHash<QString, SimulatedVariable> _variables;
    QHash<int, QTimer *> _timers;
    QElapsedTimer _clock;
    std::function<qint64()> _elapsedSource;
    QRandomGenerator _random;
    bool _running = false;
};
