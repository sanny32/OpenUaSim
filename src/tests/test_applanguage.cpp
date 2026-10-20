// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QApplication>
#include <QLocale>
#include <QSettings>
#include <QTest>
#include <QTranslator>

#include "applanguage.h"

namespace {

QString modelNotImported()
{
    return QCoreApplication::translate("MainWindow", "Model not imported");
}

}

class TestAppLanguage : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();
    void savesPreference();
    void unknownPreferenceFallsBackToSystem();
    void resolvesLocaleNames();
    void appliesRussianAndBack();
    void replacesEarlierTranslators();
    void announcesLanguageChange();
};

void TestAppLanguage::cleanup()
{
    AppLanguage::apply(*QCoreApplication::instance(), AppLanguage::Language::English);
    QSettings().remove(QStringLiteral("appearance/language"));
}

void TestAppLanguage::savesPreference()
{
    QCOMPARE(AppLanguage::saved(), AppLanguage::Language::System);
    AppLanguage::save(AppLanguage::Language::Russian);
    QCOMPARE(AppLanguage::saved(), AppLanguage::Language::Russian);
    AppLanguage::save(AppLanguage::Language::English);
    QCOMPARE(AppLanguage::saved(), AppLanguage::Language::English);
}

void TestAppLanguage::unknownPreferenceFallsBackToSystem()
{
    QSettings().setValue(QStringLiteral("appearance/language"), QStringLiteral("klingon"));
    QCOMPARE(AppLanguage::saved(), AppLanguage::Language::System);
}

void TestAppLanguage::resolvesLocaleNames()
{
    QCOMPARE(AppLanguage::localeName(AppLanguage::Language::English), QStringLiteral("en"));
    QCOMPARE(AppLanguage::localeName(AppLanguage::Language::Russian), QStringLiteral("ru"));
    QCOMPARE(AppLanguage::localeName(AppLanguage::Language::System), QLocale::system().name());
}

void TestAppLanguage::appliesRussianAndBack()
{
    QVERIFY(AppLanguage::apply(*QCoreApplication::instance(), AppLanguage::Language::Russian));
    QCOMPARE(modelNotImported(), QStringLiteral("Модель не импортирована"));
    QCOMPARE(QCoreApplication::translate("ProjectNodes", "The browse name is empty"), QStringLiteral("Имя узла пустое"));
    QCOMPARE(QCoreApplication::translate("NodeReader", "browse name is empty"), QStringLiteral("имя узла пустое"));

    QVERIFY(!AppLanguage::apply(*QCoreApplication::instance(), AppLanguage::Language::English));
    QCOMPARE(modelNotImported(), QStringLiteral("Model not imported"));
}

void TestAppLanguage::replacesEarlierTranslators()
{
    AppLanguage::apply(*QCoreApplication::instance(), AppLanguage::Language::Russian);
    const qsizetype installed = QCoreApplication::instance()->findChildren<QTranslator *>().size();
    AppLanguage::apply(*QCoreApplication::instance(), AppLanguage::Language::Russian);
    QCOMPARE(QCoreApplication::instance()->findChildren<QTranslator *>().size(), installed);
}

void TestAppLanguage::announcesLanguageChange()
{
    QWidget widget;
    widget.show();
    int changes = 0;
    class Probe : public QObject
    {
    public:
        explicit Probe(int &count) : _count(count) {}
        bool eventFilter(QObject *, QEvent *event) override
        {
            if (event->type() == QEvent::LanguageChange)
                ++_count;
            return false;
        }

    private:
        int &_count;
    } probe(changes);
    widget.installEventFilter(&probe);

    AppLanguage::apply(*QCoreApplication::instance(), AppLanguage::Language::Russian);
    QTRY_VERIFY(changes > 0);
}

QTEST_MAIN(TestAppLanguage)

#include "test_applanguage.moc"
