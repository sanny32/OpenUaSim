// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file simulationengine.cpp
/// \brief Implements the timer-driven generation of simulated variable values.
///

#include <QSet>
#include <QTimer>

#include <algorithm>

#include "signalgenerator.h"
#include "simulationengine.h"
#include "valuesink.h"

namespace {

///
/// \brief Collects the simulated variables below a set of siblings.
/// \param nodes Siblings to visit.
/// \param result Receives the variables whose function is not None.
///
void collect(const QList<NodeDefinition> &nodes, QList<SimulatedVariable> &result)
{
    for (const NodeDefinition &node : nodes) {
        if (node.kind == NodeDefinition::Kind::Variable && node.simulation.function != SimulationFunction::None)
            result.append({ node.nodeId, node.dataType, node.simulation });
        collect(node.children, result);
    }
}

}

///
/// \brief Creates a stopped engine.
/// \param sink Receiver of the values; must outlive the engine.
/// \param parent Owning QObject.
///
SimulationEngine::SimulationEngine(ValueSink *sink, QObject *parent)
    : QObject(parent)
    , _sink(sink)
    , _random(QRandomGenerator::global()->generate())
{
}

///
/// \brief Collects the simulated variables of a node tree.
/// \param nodes Project node tree.
/// \return Variables whose function is not None.
///
QList<SimulatedVariable> SimulationEngine::variablesOf(const QList<NodeDefinition> &nodes)
{
    QList<SimulatedVariable> result;
    collect(nodes, result);
    return result;
}

///
/// \brief Replaces the driven variables.
/// \param variables Variables to drive; ones with function None are ignored.
///
void SimulationEngine::setVariables(const QList<SimulatedVariable> &variables)
{
    _variables.clear();
    for (const SimulatedVariable &variable : variables) {
        if (variable.settings.function != SimulationFunction::None)
            _variables.insert(variable.nodeId, variable);
    }
    rebuildTimers();
}

///
/// \brief Adds, changes or, for function None, stops driving one variable.
/// \param variable New settings of the variable.
///
void SimulationEngine::updateVariable(const SimulatedVariable &variable)
{
    if (variable.settings.function == SimulationFunction::None)
        _variables.remove(variable.nodeId);
    else
        _variables.insert(variable.nodeId, variable);
    rebuildTimers();
}

///
/// \brief Stops driving a variable.
/// \param nodeId Variable node id.
///
void SimulationEngine::removeVariable(const QString &nodeId)
{
    if (_variables.remove(nodeId))
        rebuildTimers();
}

///
/// \brief Returns the node ids of the driven variables.
/// \return Node ids in no particular order.
///
QStringList SimulationEngine::variableIds() const
{
    return _variables.keys();
}

///
/// \brief Returns the update intervals that have a timer.
/// \return Clamped intervals in ascending order.
///
QList<int> SimulationEngine::intervals() const
{
    QList<int> result = _timers.keys();
    std::sort(result.begin(), result.end());
    return result;
}

///
/// \brief Restarts the waveforms at time zero and starts the timers.
///
void SimulationEngine::start()
{
    _clock.start();
    _running = true;
    for (QTimer *timer : std::as_const(_timers))
        timer->start();
}

///
/// \brief Stops the timers; values keep their last state.
///
void SimulationEngine::stop()
{
    _running = false;
    for (QTimer *timer : std::as_const(_timers))
        timer->stop();
}

///
/// \brief Tells whether the timers run.
/// \return True between start() and stop().
///
bool SimulationEngine::isRunning() const
{
    return _running;
}

///
/// \brief Replaces the time source, so tests control the waveform position.
/// \param elapsedMs Returns the milliseconds since start(); an empty function restores the steady clock.
///
void SimulationEngine::setElapsedSource(const std::function<qint64()> &elapsedMs)
{
    _elapsedSource = elapsedMs;
}

///
/// \brief Reseeds the noise of the Random function.
/// \param seed Seed value.
///
void SimulationEngine::setRandomSeed(quint32 seed)
{
    _random.seed(seed);
}

///
/// \brief Computes and writes the values of every variable updated at an interval.
/// \param intervalMs Clamped update interval.
///
void SimulationEngine::tick(int intervalMs)
{
    const qint64 elapsed = elapsedMs();
    QHash<QString, QVariant> values;
    for (const SimulatedVariable &variable : std::as_const(_variables)) {
        if (clampedInterval(variable.settings) != intervalMs)
            continue;
        const QVariant value = SignalGenerator::sample(variable.settings, variable.dataType, elapsed, _random);
        if (_sink->setValue(variable.nodeId, value))
            values.insert(variable.nodeId, value);
    }
    if (!values.isEmpty())
        emit valuesUpdated(values);
}

///
/// \brief Returns the update interval of a variable, clamped to the minimum.
/// \param settings Simulation settings.
/// \return Interval in milliseconds.
///
int SimulationEngine::clampedInterval(const SimulationSettings &settings)
{
    return qMax(kMinimumIntervalMs, settings.intervalMs);
}

///
/// \brief Returns the waveform position.
/// \return Milliseconds since start(), from the injected source when there is one.
///
qint64 SimulationEngine::elapsedMs() const
{
    if (_elapsedSource)
        return _elapsedSource();
    return _clock.isValid() ? _clock.elapsed() : 0;
}

///
/// \brief Keeps exactly one timer per interval in use, running when the engine runs.
///
void SimulationEngine::rebuildTimers()
{
    QSet<int> needed;
    for (const SimulatedVariable &variable : std::as_const(_variables))
        needed.insert(clampedInterval(variable.settings));

    for (auto it = _timers.begin(); it != _timers.end();) {
        if (needed.contains(it.key())) {
            ++it;
            continue;
        }
        delete it.value();
        it = _timers.erase(it);
    }
    for (int interval : std::as_const(needed)) {
        if (_timers.contains(interval))
            continue;
        auto *timer = new QTimer(this);
        timer->setInterval(interval);
        timer->setTimerType(interval < 1000 ? Qt::PreciseTimer : Qt::CoarseTimer);
        connect(timer, &QTimer::timeout, this, &SimulationEngine::handleTimeout);
        _timers.insert(interval, timer);
        if (_running)
            timer->start();
    }
}

///
/// \brief Ticks the interval of the timer that fired.
///
void SimulationEngine::handleTimeout()
{
    if (const auto *timer = qobject_cast<const QTimer *>(sender()))
        tick(timer->interval());
}
