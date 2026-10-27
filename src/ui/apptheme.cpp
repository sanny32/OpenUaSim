// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file apptheme.cpp
/// \brief Implements the application light/dark theme manager.
///

#include <QApplication>
#include <QMetaEnum>
#include <QStyleHints>

#include "apppalette.h"
#include "apptheme.h"
#include "settingsstore.h"

namespace {

const char *const kModeKey = "appearance/theme";

}

///
/// \brief Constructs the theme manager and starts following system scheme changes.
/// \param parent Owning QObject.
///
AppTheme::AppTheme(QObject *parent)
    : QObject(parent)
{
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
            this, &AppTheme::applyPalette);
}

///
/// \brief Returns the colour scheme preference.
/// \return Current preference.
///
AppTheme::Mode AppTheme::mode() const
{
    return _mode;
}

///
/// \brief Applies and persists a colour scheme preference.
/// \param mode Light or Dark to override the system, System to follow it.
///
void AppTheme::setMode(Mode mode)
{
    _mode = mode;
    SettingsStore().setValue(kModeKey, QString::fromLatin1(QMetaEnum::fromType<Mode>().valueToKey(int(mode))));
    apply();
}

///
/// \brief Applies the persisted preference; the system scheme when none is saved.
///
void AppTheme::applySavedMode()
{
    const QByteArray key = SettingsStore().value(kModeKey).toString().toLatin1();
    bool ok = false;
    const int value = QMetaEnum::fromType<Mode>().keyToValue(key.constData(), &ok);
    _mode = ok ? static_cast<Mode>(value) : Mode::System;
    apply();
}

///
/// \brief Reports whether the dark palette is active.
/// \return True when the dark palette is applied.
///
bool AppTheme::isDark() const
{
    return _dark;
}

///
/// \brief Forwards the preference to Qt, which also themes the native title bar, then
///        applies the matching palette.
///
void AppTheme::apply()
{
    Qt::ColorScheme scheme = Qt::ColorScheme::Unknown;
    if (_mode == Mode::Light)
        scheme = Qt::ColorScheme::Light;
    else if (_mode == Mode::Dark)
        scheme = Qt::ColorScheme::Dark;
    QGuiApplication::styleHints()->setColorScheme(scheme);
    applyPalette();
}

///
/// \brief Installs the palette for the effective colour scheme when it changed.
///
void AppTheme::applyPalette()
{
    bool dark = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
    if (_mode != Mode::System)
        dark = _mode == Mode::Dark;
    if (_applied && dark == _dark)
        return;

    _dark = dark;
    _applied = true;
    QApplication::setPalette(AppPalette::build(dark));
    emit schemeChanged(dark);
}
