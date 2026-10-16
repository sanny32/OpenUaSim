// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file apptheme.h
/// \brief Declares the application light/dark theme manager.
///

#pragma once

#include <QObject>

///
/// \brief Applies the light or dark palette, following the system or a user choice.
///
class AppTheme : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Colour scheme preference.
    ///
    enum class Mode { System, Light, Dark };
    Q_ENUM(Mode)

    ///
    /// \brief Constructs the theme manager and starts following system scheme changes.
    /// \param parent Owning QObject.
    ///
    explicit AppTheme(QObject *parent = nullptr);

    ///
    /// \brief Returns the colour scheme preference.
    /// \return Current preference.
    ///
    Mode mode() const;

    ///
    /// \brief Applies and persists a colour scheme preference.
    /// \param mode Light or Dark to override the system, System to follow it.
    ///
    void setMode(Mode mode);

    ///
    /// \brief Applies the persisted preference; the system scheme when none is saved.
    ///
    void applySavedMode();

    ///
    /// \brief Reports whether the dark palette is active.
    /// \return True when the dark palette is applied.
    ///
    bool isDark() const;

signals:
    ///
    /// \brief Emitted after the active palette switched between light and dark.
    /// \param dark True when the dark palette is now active.
    ///
    void schemeChanged(bool dark);

private:
    void apply();
    void applyPalette();

    Mode _mode = Mode::System;
    bool _dark = false;
    bool _applied = false;
};
