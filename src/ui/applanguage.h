// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file applanguage.h
/// \brief Declares the choice and loading of the user interface language.
///

#pragma once

#include <QList>
#include <QString>

class QCoreApplication;

namespace AppLanguage {

///
/// \brief User interface language preference.
///
enum class Language { System, English, Russian };

///
/// \brief Returns every language preference in menu order.
/// \return System first, then the translations shipped with the application.
///
QList<Language> languages();

///
/// \brief Returns the persisted language preference.
/// \return Saved preference; System when none or an unknown one is saved.
///
Language saved();

///
/// \brief Persists a language preference for the next start.
/// \param language Preference to save.
///
void save(Language language);

///
/// \brief Returns the locale name of a language preference.
/// \param language Preference to resolve.
/// \return "en" or "ru", or the system locale name for System.
///
QString localeName(Language language);

///
/// \brief Replaces the installed translations with those of a language preference; widgets then
///        receive QEvent::LanguageChange and retranslate themselves.
/// \param app Application receiving the translators, which it owns.
/// \param language Preference to apply; English and untranslated system languages install none.
/// \return True when an application translation was installed.
///
bool apply(QCoreApplication &app, Language language);

}
