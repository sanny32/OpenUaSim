// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#pragma once

class QSettings;
class QWidget;

namespace WidgetLayout {

///
/// \brief Saves named splitters and column headers without including child windows.
/// \param root Window whose layout is saved.
/// \param settings Settings group reserved for this window.
///
void save(const QWidget &root, QSettings &settings);

///
/// \brief Restores named splitters and column headers, retaining defaults for missing or invalid state.
/// \param root Window whose layout is restored.
/// \param settings Settings group reserved for this window.
///
void restore(QWidget &root, QSettings &settings);

}
