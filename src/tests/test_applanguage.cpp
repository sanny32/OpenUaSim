// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QLocale>
#include <QSettings>
#include <QTest>
#include <QTranslator>

#include "applanguage.h"
#include "addnodedialog.h"
#include "project/projectfile.h"
#include "variablepropertieswidget.h"

Q_DECLARE_METATYPE(VariableInfo)

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
    void accessValuesStayPortable_data();
    void accessValuesStayPortable();
    void savesPreference();
    void unknownPreferenceFallsBackToSystem();
    void resolvesLocaleNames();
    void appliesRussianAndBack();
    void replacesEarlierTranslators();
    void announcesLanguageChange();
};

void TestAppLanguage::accessValuesStayPortable_data()
{
    QTest::addColumn<QString>("access");
    QTest::newRow("read") << QStringLiteral("Read");
    QTest::newRow("write") << QStringLiteral("Write");
    QTest::newRow("readWrite") << QStringLiteral("Read/Write");
}

void TestAppLanguage::accessValuesStayPortable()
{
    QFETCH(QString, access);
    AppLanguage::apply(*QCoreApplication::instance(), AppLanguage::Language::Russian);
    AddNodeDialog dialog({}, {});
    dialog.findChild<QComboBox *>(QStringLiteral("kindCombo"))->setCurrentIndex(int(NodeDefinition::Kind::Variable));
    dialog.findChild<QLineEdit *>(QStringLiteral("browseNameEdit"))->setText(QStringLiteral("AI"));
    auto *createdAccess = dialog.findChild<QComboBox *>(QStringLiteral("accessCombo"));
    createdAccess->setCurrentIndex(createdAccess->findData(access));
    QVERIFY(createdAccess->currentIndex() >= 0);
    QVERIFY(createdAccess->currentText() != access);
    QCOMPARE(dialog.node().access, access);
    ProjectConfig project;
    project.nodes.append(dialog.node());
    const auto loaded = ProjectFile::fromJson(ProjectFile::toJson(project));
    QVERIFY2(loaded.ok(), qPrintable(loaded.error));
    QCOMPARE(loaded.project.nodes.first().access, access);

    VariableInfo variable;
    variable.nodeId = QStringLiteral("ns=1;s=AI");
    variable.displayName = QStringLiteral("AI");
    variable.dataType = QStringLiteral("Double");
    variable.access = access;
    VariablePropertiesWidget editor;
    editor.setVariable(variable);
    auto *editedAccess = editor.findChild<QComboBox *>(QStringLiteral("accessCombo"));
    QCOMPARE(editedAccess->currentData().toString(), access);
    QVERIFY(editedAccess->currentText() != access);
    QSignalSpy applied(&editor, &VariablePropertiesWidget::applyRequested);
    editor.findChild<QPushButton *>(QStringLiteral("applyButton"))->click();
    QCOMPARE(applied.size(), 1);
    QCOMPARE(applied.takeFirst().first().value<VariableInfo>().access, access);
    const int changedIndex = (editedAccess->currentIndex() + 1) % editedAccess->count();
    editedAccess->setCurrentIndex(changedIndex);
    const QString changedAccess = editedAccess->currentData().toString();
    editor.findChild<QPushButton *>(QStringLiteral("applyButton"))->click();
    QCOMPARE(applied.takeFirst().first().value<VariableInfo>().access, changedAccess);
    AppLanguage::apply(*QCoreApplication::instance(), AppLanguage::Language::English);
    QCoreApplication::processEvents();
    QCOMPARE(editedAccess->currentText(), changedAccess);
    QCOMPARE(editedAccess->currentData().toString(), changedAccess);
    editor.setVariable(variable);
    QCOMPARE(editedAccess->currentData().toString(), access);
    AppLanguage::apply(*QCoreApplication::instance(), AppLanguage::Language::Russian);
    QCoreApplication::processEvents();
    QCOMPARE(editedAccess->currentData().toString(), access);
    editor.findChild<QPushButton *>(QStringLiteral("applyButton"))->click();
    QCOMPARE(applied.takeFirst().first().value<VariableInfo>().access, access);
}

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
