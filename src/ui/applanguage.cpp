// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file applanguage.cpp
/// \brief Implements the choice and loading of the user interface language.
///

#include <QCoreApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
#include <QTranslator>

#include "applanguage.h"

namespace {

const char *const kLanguageKey = "appearance/language";
const char *const kTranslatorName = "OpenUaSimTranslator";

///
/// \brief Returns the stored name of a language preference.
/// \param language Preference to name.
/// \return "system", "en" or "ru".
///
QString key(AppLanguage::Language language)
{
    switch (language) {
    case AppLanguage::Language::System:  return QStringLiteral("system");
    case AppLanguage::Language::English: return QStringLiteral("en");
    case AppLanguage::Language::Russian: return QStringLiteral("ru");
    }
    return {};
}

///
/// \brief Loads a translation file for a locale and installs it.
/// \param app Application receiving and owning the translator.
/// \param locale Locale whose UI languages are tried in order.
/// \param fileName File name stem, e.g. "openuasim" for "openuasim_ru.qm".
/// \param directory Directory or resource path holding the files.
/// \return True when a file was found and installed.
///
bool installTranslator(QCoreApplication &app, const QLocale &locale, const QString &fileName, const QString &directory)
{
    auto *translator = new QTranslator(&app);
    translator->setObjectName(QLatin1String(kTranslatorName));
    if (translator->load(locale, fileName, QStringLiteral("_"), directory) && app.installTranslator(translator))
        return true;
    delete translator;
    return false;
}

}

namespace AppLanguage {

///
/// \brief Returns every language preference in menu order.
/// \return System first, then the translations shipped with the application.
///
QList<Language> languages()
{
    return { Language::System, Language::English, Language::Russian };
}

///
/// \brief Returns the persisted language preference.
/// \return Saved preference; System when none or an unknown one is saved.
///
Language saved()
{
    const QString stored = QSettings().value(kLanguageKey).toString();
    for (Language language : languages()) {
        if (key(language) == stored)
            return language;
    }
    return Language::System;
}

///
/// \brief Persists a language preference for the next start.
/// \param language Preference to save.
///
void save(Language language)
{
    QSettings().setValue(kLanguageKey, key(language));
}

///
/// \brief Returns the locale name of a language preference.
/// \param language Preference to resolve.
/// \return "en" or "ru", or the system locale name for System.
///
QString localeName(Language language)
{
    return language == Language::System ? QLocale::system().name() : key(language);
}

///
/// \brief Replaces the installed translations with those of a language preference; widgets then
///        receive QEvent::LanguageChange and retranslate themselves.
/// \param app Application receiving the translators, which it owns.
/// \param language Preference to apply; English and untranslated system languages install none.
/// \return True when an application translation was installed.
///
bool apply(QCoreApplication &app, Language language)
{
    const QList<QTranslator *> installed = app.findChildren<QTranslator *>(QLatin1String(kTranslatorName),
                                                                           Qt::FindDirectChildrenOnly);
    for (QTranslator *translator : installed) {
        app.removeTranslator(translator);
        delete translator;
    }
    const QLocale locale(localeName(language));
    if (locale.language() == QLocale::English)
        return false;
    if (!installTranslator(app, locale, QStringLiteral("openuasim"), QStringLiteral(":/i18n")))
        return false;
    installTranslator(app, locale, QStringLiteral("qtbase"), QLibraryInfo::path(QLibraryInfo::TranslationsPath));
    return true;
}

}
