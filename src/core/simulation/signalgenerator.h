// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file signalgenerator.h
/// \brief Declares the waveforms that drive simulated variables.
///

#pragma once

#include <QList>
#include <QString>

#include "data/variableinfo.h"

class QRandomGenerator;

namespace SignalGenerator {

///
/// \brief Returns every simulation function in display order.
/// \return Functions starting with None.
///
QList<SimulationFunction> functions();

///
/// \brief Returns the user-facing name of a function.
/// \param function Function to name.
/// \return Translated name such as "Sine wave".
///
QString displayName(SimulationFunction function);

///
/// \brief Returns the stable identifier a function is stored under in project files.
/// \param function Function to name.
/// \return Key such as "sine".
///
QString key(SimulationFunction function);

///
/// \brief Parses a stored function identifier.
/// \param key Identifier written by key().
/// \param ok Set to whether the key is known; may be null.
/// \return Matching function, or None for an unknown key.
///
SimulationFunction fromKey(const QString &key, bool *ok = nullptr);

///
/// \brief Computes the simulated value at a point in time.
/// \param settings Waveform, period, amplitude and offset.
/// \param elapsedMs Time since the simulation started, in milliseconds.
/// \param random Source of noise for the Random function.
/// \return Value between offset - amplitude and offset + amplitude; offset for None.
///
double value(const SimulationSettings &settings, qint64 elapsedMs, QRandomGenerator &random);

}
