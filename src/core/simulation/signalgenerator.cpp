// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file signalgenerator.cpp
/// \brief Implements the waveforms that drive simulated variables.
///

#include <QCoreApplication>
#include <QRandomGenerator>
#include <QtMath>

#include "signalgenerator.h"
#include "valuecast.h"

namespace {

constexpr int kStepLevels = 5;

///
/// \brief Returns how far into the current cycle a point in time is.
/// \param elapsedMs Time since the simulation started.
/// \param periodMs Cycle length; non-positive lengths are treated as one millisecond.
/// \return Phase in [0, 1).
///
double phase(qint64 elapsedMs, int periodMs)
{
    const qint64 period = qMax(1, periodMs);
    const qint64 position = ((elapsedMs % period) + period) % period;
    return double(position) / double(period);
}

///
/// \brief Computes a unit waveform for a phase.
/// \param function Waveform.
/// \param phase Phase in [0, 1).
/// \param random Noise source for Random.
/// \return Value in [-1, 1].
///
double unitValue(SimulationFunction function, double phase, QRandomGenerator &random)
{
    switch (function) {
    case SimulationFunction::None:
        return 0.0;
    case SimulationFunction::Sine:
        return qSin(2.0 * M_PI * phase);
    case SimulationFunction::Square:
        return phase < 0.5 ? 1.0 : -1.0;
    case SimulationFunction::Triangle:
        return phase < 0.5 ? 4.0 * phase - 1.0 : 3.0 - 4.0 * phase;
    case SimulationFunction::Ramp:
        return 2.0 * phase - 1.0;
    case SimulationFunction::Random:
        return 2.0 * random.generateDouble() - 1.0;
    case SimulationFunction::Step: {
        const int level = qMin(kStepLevels - 1, int(phase * kStepLevels));
        return -1.0 + 2.0 * level / (kStepLevels - 1);
    }
    case SimulationFunction::Toggle:
    case SimulationFunction::Counter:
    case SimulationFunction::List:
        break;
    }
    return 0.0;
}

///
/// \brief Counts the updates since the simulation started.
/// \param settings Settings providing the update interval.
/// \param elapsedMs Time since the simulation started.
/// \return Number of whole update intervals; zero before the start.
///
qint64 updateCount(const SimulationSettings &settings, qint64 elapsedMs)
{
    return qMax<qint64>(0, elapsedMs) / qMax(1, settings.intervalMs);
}

///
/// \brief Counts the updates since the current period started.
/// \param settings Settings providing the update interval and period.
/// \param elapsedMs Time since the simulation started.
/// \return Number of whole update intervals within the period; a period shorter than the interval never wraps.
///
qint64 updateCountInPeriod(const SimulationSettings &settings, qint64 elapsedMs)
{
    const qint64 elapsed = qMax<qint64>(0, elapsedMs);
    return updateCount(settings, settings.periodMs >= settings.intervalMs ? elapsed % qMax(1, settings.periodMs)
                                                                          : elapsed);
}

}

namespace SignalGenerator {

///
/// \brief Returns every simulation function in display order.
/// \return Functions starting with None.
///
QList<SimulationFunction> functions()
{
    return { SimulationFunction::None, SimulationFunction::Sine, SimulationFunction::Square,
             SimulationFunction::Triangle, SimulationFunction::Ramp, SimulationFunction::Random,
             SimulationFunction::Step, SimulationFunction::Toggle, SimulationFunction::Counter,
             SimulationFunction::List };
}

///
/// \brief Returns the user-facing name of a function.
/// \param function Function to name.
/// \return Translated name such as "Sine wave".
///
QString displayName(SimulationFunction function)
{
    switch (function) {
    case SimulationFunction::None:     return QCoreApplication::translate("SignalGenerator", "None");
    case SimulationFunction::Sine:     return QCoreApplication::translate("SignalGenerator", "Sine wave");
    case SimulationFunction::Square:   return QCoreApplication::translate("SignalGenerator", "Square wave");
    case SimulationFunction::Triangle: return QCoreApplication::translate("SignalGenerator", "Triangle wave");
    case SimulationFunction::Ramp:     return QCoreApplication::translate("SignalGenerator", "Ramp");
    case SimulationFunction::Random:   return QCoreApplication::translate("SignalGenerator", "Random");
    case SimulationFunction::Step:     return QCoreApplication::translate("SignalGenerator", "Step");
    case SimulationFunction::Toggle:   return QCoreApplication::translate("SignalGenerator", "Toggle");
    case SimulationFunction::Counter:  return QCoreApplication::translate("SignalGenerator", "Counter");
    case SimulationFunction::List:     return QCoreApplication::translate("SignalGenerator", "Value list");
    }
    return {};
}

///
/// \brief Returns the stable identifier a function is stored under in project files.
/// \param function Function to name.
/// \return Key such as "sine".
///
QString key(SimulationFunction function)
{
    switch (function) {
    case SimulationFunction::None:     return QStringLiteral("none");
    case SimulationFunction::Sine:     return QStringLiteral("sine");
    case SimulationFunction::Square:   return QStringLiteral("square");
    case SimulationFunction::Triangle: return QStringLiteral("triangle");
    case SimulationFunction::Ramp:     return QStringLiteral("ramp");
    case SimulationFunction::Random:   return QStringLiteral("random");
    case SimulationFunction::Step:     return QStringLiteral("step");
    case SimulationFunction::Toggle:   return QStringLiteral("toggle");
    case SimulationFunction::Counter:  return QStringLiteral("counter");
    case SimulationFunction::List:     return QStringLiteral("list");
    }
    return {};
}

///
/// \brief Parses a stored function identifier.
/// \param key Identifier written by key().
/// \param ok Set to whether the key is known; may be null.
/// \return Matching function, or None for an unknown key.
///
SimulationFunction fromKey(const QString &key, bool *ok)
{
    for (SimulationFunction function : functions()) {
        if (SignalGenerator::key(function) == key) {
            if (ok)
                *ok = true;
            return function;
        }
    }
    if (ok)
        *ok = false;
    return SimulationFunction::None;
}

///
/// \brief Computes the simulated value at a point in time; Step climbs through five evenly spaced levels per period.
/// \param settings Waveform, interval, period, amplitude and offset.
/// \param elapsedMs Time since the simulation started, in milliseconds.
/// \param random Source of noise for the Random function.
/// \return Value between offset - amplitude and offset + amplitude for the waveforms; Toggle alternates
///         between offset and offset + amplitude on every update, Counter adds amplitude on every update
///         and restarts each period; offset for None and List.
///
double value(const SimulationSettings &settings, qint64 elapsedMs, QRandomGenerator &random)
{
    switch (settings.function) {
    case SimulationFunction::Toggle:
        return settings.offset + settings.amplitude * double(updateCount(settings, elapsedMs) % 2);
    case SimulationFunction::Counter:
        return settings.offset + settings.amplitude * double(updateCountInPeriod(settings, elapsedMs));
    default:
        break;
    }
    const double unit = unitValue(settings.function, phase(elapsedMs, settings.periodMs), random);
    return settings.offset + settings.amplitude * unit;
}

///
/// \brief Computes the simulated value at a point in time as a value of a data type.
/// \param settings Function and its parameters.
/// \param dataType Built-in data type name.
/// \param elapsedMs Time since the simulation started, in milliseconds.
/// \param random Source of noise for the Random function.
/// \return List takes the next entry of its values on every update, parsed as the data type, or the type's
///         default for an entry that does not parse or an empty list; other functions convert value().
///
QVariant sample(const SimulationSettings &settings, const QString &dataType, qint64 elapsedMs,
                QRandomGenerator &random)
{
    if (settings.function != SimulationFunction::List)
        return ValueCast::fromDouble(value(settings, elapsedMs, random), dataType);
    if (settings.values.isEmpty())
        return ValueCast::defaultValue(dataType);
    const QString &text = settings.values.at(updateCount(settings, elapsedMs) % settings.values.size());
    const QVariant parsed = ValueCast::fromText(text, dataType);
    return parsed.isValid() ? parsed : ValueCast::defaultValue(dataType);
}

}
