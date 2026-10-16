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
    }
    return 0.0;
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
             SimulationFunction::Step };
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
/// \param settings Waveform, period, amplitude and offset.
/// \param elapsedMs Time since the simulation started, in milliseconds.
/// \param random Source of noise for the Random function.
/// \return Value between offset - amplitude and offset + amplitude; offset for None.
///
double value(const SimulationSettings &settings, qint64 elapsedMs, QRandomGenerator &random)
{
    const double unit = unitValue(settings.function, phase(elapsedMs, settings.periodMs), random);
    return settings.offset + settings.amplitude * unit;
}

}
