// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QComboBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalSpy>
#include <QTableWidget>
#include <QTest>
#include <QTimer>
#include <QToolButton>

#include "applanguage.h"
#include "apptheme.h"
#include "appstyle.h"
#include "methodsourcedialog.h"
#include "methodpropertieswidget.h"

Q_DECLARE_METATYPE(MethodInfo)

namespace {

/// \brief Builds a draft project method with one scalar argument on each side.
MethodInfo method()
{
    MethodInfo info;
    info.nodeId = QStringLiteral("ns=1;s=Run");
    info.kind = MethodInfo::Kind::Project;
    info.action = NodeDefinition::MethodAction::Scenario;
    info.namespaces = { QStringLiteral("http://opcfoundation.org/UA/"), QStringLiteral("urn:test") };
    info.inputs = { { QStringLiteral("Value"), QStringLiteral("Double"), QStringLiteral("Double"), false, {} } };
    info.outputs = info.inputs;
    return info;
}

}

class TestMethodScenarioEditor : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanup();
    void fitsNarrowPanel_data();
    void fitsNarrowPanel();
    void editsSignatureAndActionOrder();
    void revertsDraftChanges();
    void importedSignatureIsReadOnly();
    void removingInputKeepsOtherReferences();
    void sourceDialogPreservesEachKind_data();
    void sourceDialogPreservesEachKind();
    void doubleClickEditsSource_data();
    void doubleClickEditsSource();

private:
    void configureSourceDialog();
    bool _acceptSource = false;
    int _sourceDialogs = 0;
};

void TestMethodScenarioEditor::initTestCase()
{
    qApp->setStyle(new AppStyle);
#ifdef Q_OS_WIN
    qApp->setFont(QFont(QStringLiteral("Segoe UI"), 9));
#endif
    AppTheme theme;
    theme.setMode(AppTheme::Mode::Light);
}

void TestMethodScenarioEditor::cleanup()
{
    AppLanguage::apply(*qApp, AppLanguage::Language::English);
}

void TestMethodScenarioEditor::fitsNarrowPanel_data()
{
    QTest::addColumn<bool>("russian");
    QTest::addColumn<bool>("imported");
    QTest::addColumn<int>("width");
    for (bool russian : { false, true }) {
        for (bool imported : { false, true }) {
            for (int width : { 320, 400, 500 }) {
                const QString row = QStringLiteral("%1-%2-%3").arg(russian ? "ru" : "en")
                    .arg(imported ? "model" : "project").arg(width);
                QTest::newRow(qPrintable(row)) << russian << imported << width;
            }
        }
    }
}

void TestMethodScenarioEditor::fitsNarrowPanel()
{
    QFETCH(bool, russian);
    QFETCH(bool, imported);
    QFETCH(int, width);
    AppLanguage::apply(*qApp, russian ? AppLanguage::Language::Russian : AppLanguage::Language::English);
    MethodInfo info = method();
    if (imported)
        info.kind = MethodInfo::Kind::Model;
    MethodValueSource source;
    source.kind = MethodValueSource::Kind::Variable;
    source.variableId = QStringLiteral("nsu=urn:test;s=Machine.With.A.Long.Variable.Identifier");
    info.response.sources = { source };
    info.response.actions = { { true, source.variableId, source } };
    MethodPropertiesWidget widget;
    widget.setMethod(info);
    widget.resize(width, 720);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.width(), width);
    auto *scroll = widget.findChild<QScrollArea *>(QStringLiteral("methodScroll"));
    QTRY_VERIFY(scroll->widget()->width() <= scroll->viewport()->width());
    QCOMPARE(scroll->horizontalScrollBar()->maximum(), 0);
    QVERIFY(!widget.findChild<QAbstractButton *>(QStringLiteral("methodEditOutputSourceButton")));
    QVERIFY(!widget.findChild<QAbstractButton *>(QStringLiteral("methodEditActionSourceButton")));
    for (const char *name : { "methodAddInputButton", "methodAddOutputButton", "methodAddActionButton",
                              "methodRemoveInputButton", "methodRemoveOutputButton", "methodRemoveActionButton" }) {
        auto *button = widget.findChild<QToolButton *>(QLatin1String(name));
        QVERIFY2(button, name);
        QVERIFY(!button->text().isEmpty());
        QCOMPARE(button->toolButtonStyle(), Qt::ToolButtonTextBesideIcon);
        if (button->isVisible()) {
            const QRect bounds(button->mapTo(scroll->widget(), QPoint()), button->size());
            QVERIFY2(bounds.left() >= 0 && bounds.right() < scroll->widget()->width(), name);
        }
    }
    for (const char *name : { "methodActionUpButton", "methodActionDownButton" }) {
        auto *button = widget.findChild<QToolButton *>(QLatin1String(name));
        QCOMPARE(button->width(), button->height());
        QCOMPARE(button->height(), widget.findChild<QToolButton *>(QStringLiteral("methodAddActionButton"))->height());
        QVERIFY(button->width() < widget.findChild<QToolButton *>(QStringLiteral("methodAddActionButton"))->width());
    }
    QVERIFY(!widget.findChild<QFrame *>(QStringLiteral("methodActionButtonsSeparator")));
    QVERIFY(!widget.findChild<QFrame *>(QStringLiteral("methodActionButtonsGroup")));
    for (const char *name : { "methodAddActionButton", "methodRemoveActionButton" }) {
        auto *button = widget.findChild<QToolButton *>(QLatin1String(name));
        // Qt may compress fixed-policy buttons when translated labels exhaust the available width.
        QVERIFY(button->width() <= button->sizeHint().width());
        const int contentWidth = button->fontMetrics().horizontalAdvance(button->text()) + button->iconSize().width();
        QVERIFY2(button->width() >= contentWidth, qPrintable(button->objectName()));
        QCOMPARE(button->height(), 26);
    }
    for (const char *layoutName : { "inputButtonsLayout", "outputButtonsLayout", "setActionButtonsLayout" }) {
        auto *layout = widget.findChild<QHBoxLayout *>(QLatin1String(layoutName));
        QVERIFY2(layout, layoutName);
        int centerY = -1;
        QWidget *first = nullptr;
        QWidget *last = nullptr;
        for (int index = 0; index < layout->count(); ++index) {
            QWidget *button = layout->itemAt(index)->widget();
            if (!button || !button->isVisible())
                continue;
            if (centerY < 0)
                centerY = button->geometry().center().y();
            QCOMPARE(button->geometry().center().y(), centerY);
            if (!first)
                first = button;
            last = button;
        }
        if (first) {
            QCOMPARE(first->geometry().left(), layout->contentsRect().left());
            QCOMPARE(last->geometry().right(), layout->contentsRect().right());
        }
    }
    for (QTableWidget *table : widget.findChildren<QTableWidget *>())
        QTRY_VERIFY2(table->horizontalScrollBar()->maximum() == 0,
                     qPrintable(QStringLiteral("%1: sections=%2 viewport=%3 mode=%4 stretchLast=%5")
                         .arg(table->objectName()).arg(table->horizontalHeader()->length())
                         .arg(table->viewport()->width()).arg(table->horizontalHeader()->sectionResizeMode(0))
                         .arg(table->horizontalHeader()->stretchLastSection())));
    for (QToolButton *button : widget.findChildren<QToolButton *>()) {
        if (!button->isVisible())
            continue;
        QVERIFY(!button->toolTip().isEmpty());
        const QRect bounds(button->mapTo(scroll->widget(), QPoint()), button->size());
        QVERIFY2(bounds.left() >= 0 && bounds.right() < scroll->widget()->width(), qPrintable(button->objectName()));
    }
}

void TestMethodScenarioEditor::editsSignatureAndActionOrder()
{
    MethodPropertiesWidget widget;
    widget.setMethod(method());
    auto *inputs = widget.findChild<QTableWidget *>(QStringLiteral("methodInputsTable"));
    QVERIFY(inputs->item(0, 0)->flags().testFlag(Qt::ItemIsEditable));
    widget.findChild<QAbstractButton *>(QStringLiteral("methodAddInputButton"))->click();
    inputs->item(1, 0)->setText(QStringLiteral("Counts"));
    inputs->item(1, 1)->setText(QStringLiteral("UInt32[]"));
    widget.findChild<QAbstractButton *>(QStringLiteral("methodAddActionButton"))->click();
    auto *actions = widget.findChild<QTableWidget *>(QStringLiteral("methodSetActionsTable"));
    actions->item(0, 1)->setText(QStringLiteral("ns=1;s=A"));
    widget.findChild<QAbstractButton *>(QStringLiteral("methodAddActionButton"))->click();
    actions->item(1, 1)->setText(QStringLiteral("ns=1;s=B"));
    actions->item(1, 0)->setCheckState(Qt::Unchecked);
    widget.findChild<QAbstractButton *>(QStringLiteral("methodActionUpButton"))->click();
    QSignalSpy requested(&widget, &MethodPropertiesWidget::applyRequested);
    widget.findChild<QPushButton *>(QStringLiteral("methodApplyButton"))->click();
    QCOMPARE(requested.count(), 1);
    const MethodInfo result = requested.first().first().value<MethodInfo>();
    QCOMPARE(result.inputs.size(), 2);
    QCOMPARE(result.inputs.last().valueType, QStringLiteral("UInt32"));
    QVERIFY(result.inputs.last().array);
    QCOMPARE(result.response.actions.size(), 2);
    QCOMPARE(result.response.actions.first().targetId, QStringLiteral("ns=1;s=B"));
    QVERIFY(!result.response.actions.first().enabled);
    QCOMPARE(result.response.actions.last().targetId, QStringLiteral("ns=1;s=A"));
}

void TestMethodScenarioEditor::revertsDraftChanges()
{
    MethodPropertiesWidget widget;
    widget.setMethod(method());
    widget.findChild<QAbstractButton *>(QStringLiteral("methodAddActionButton"))->click();
    widget.findChild<QAbstractButton *>(QStringLiteral("methodAddOutputButton"))->click();
    widget.findChild<QPushButton *>(QStringLiteral("methodRevertButton"))->click();
    QCOMPARE(widget.findChild<QTableWidget *>(QStringLiteral("methodSetActionsTable"))->rowCount(), 0);
    QCOMPARE(widget.findChild<QTableWidget *>(QStringLiteral("methodOutputsTable"))->rowCount(), 1);
    QVERIFY(!widget.findChild<QPushButton *>(QStringLiteral("methodApplyButton"))->isEnabled());
}

void TestMethodScenarioEditor::importedSignatureIsReadOnly()
{
    MethodInfo info = method();
    info.kind = MethodInfo::Kind::Model;
    MethodPropertiesWidget widget;
    widget.setMethod(info);
    auto *inputs = widget.findChild<QTableWidget *>(QStringLiteral("methodInputsTable"));
    auto *outputs = widget.findChild<QTableWidget *>(QStringLiteral("methodOutputsTable"));
    QVERIFY(!inputs->item(0, 0)->flags().testFlag(Qt::ItemIsEditable));
    QVERIFY(!outputs->item(0, 1)->flags().testFlag(Qt::ItemIsEditable));
    QVERIFY(outputs->item(0, 2)->flags().testFlag(Qt::ItemIsEditable));
    QVERIFY(widget.findChild<QWidget *>(QStringLiteral("methodInputButtons"))->isHidden());
    QVERIFY(!widget.findChild<QWidget *>(QStringLiteral("methodScenarioActions"))->isHidden());
}

void TestMethodScenarioEditor::removingInputKeepsOtherReferences()
{
    MethodInfo info = method();
    info.inputs.append({ QStringLiteral("Second"), QStringLiteral("Double"), QStringLiteral("Double"), false, {} });
    MethodValueSource source;
    source.kind = MethodValueSource::Kind::Input;
    source.inputIndex = 1;
    info.response.sources = { source };
    info.response.actions = { { true, QStringLiteral("ns=1;s=A"), source } };
    MethodPropertiesWidget widget;
    widget.setMethod(info);
    auto *inputs = widget.findChild<QTableWidget *>(QStringLiteral("methodInputsTable"));
    inputs->setCurrentCell(0, 0);
    widget.findChild<QAbstractButton *>(QStringLiteral("methodRemoveInputButton"))->click();
    QSignalSpy requested(&widget, &MethodPropertiesWidget::applyRequested);
    widget.findChild<QPushButton *>(QStringLiteral("methodApplyButton"))->click();
    QCOMPARE(requested.count(), 1);
    const MethodInfo result = requested.first().first().value<MethodInfo>();
    QCOMPARE(result.inputs.first().name, QStringLiteral("Second"));
    QCOMPARE(result.response.sources.first().inputIndex, 0);
    QCOMPARE(result.response.actions.first().source.inputIndex, 0);
    inputs->setCurrentCell(0, 0);
    widget.findChild<QAbstractButton *>(QStringLiteral("methodRemoveInputButton"))->click();
    widget.findChild<QPushButton *>(QStringLiteral("methodApplyButton"))->click();
    QCOMPARE(requested.last().first().value<MethodInfo>().response.sources.first().inputIndex, -1);
}

void TestMethodScenarioEditor::sourceDialogPreservesEachKind_data()
{
    QTest::addColumn<int>("kind");
    for (int index = 0; index < 4; ++index)
        QTest::newRow(qPrintable(QString::number(index))) << index;
}

void TestMethodScenarioEditor::sourceDialogPreservesEachKind()
{
    QFETCH(int, kind);
    MethodValueSource source;
    source.kind = MethodValueSource::Kind(kind);
    source.value = QStringLiteral("2; 3");
    source.variableId = QStringLiteral("nsu=urn:test;s=A");
    source.simulation.function = SimulationFunction::Counter;
    source.simulation.amplitude = 3;
    source.simulation.offset = 4;
    MethodSourceDialog dialog(source, method(), {});
    const MethodValueSource result = dialog.source();
    QCOMPARE(result.kind, source.kind);
    QCOMPARE(result.value, source.value);
    QCOMPARE(result.variableId, source.variableId);
    QCOMPARE(result.inputIndex, source.inputIndex);
    QCOMPARE(result.simulation.function, source.simulation.function);
    QCOMPARE(result.simulation.amplitude, source.simulation.amplitude);
    QVERIFY(dialog.findChild<QLineEdit *>(QStringLiteral("constantEdit"))->isEnabled() == (kind == 0));
    QVERIFY(dialog.findChild<QComboBox *>(QStringLiteral("inputCombo"))->isEnabled() == (kind == 1));
}


void TestMethodScenarioEditor::doubleClickEditsSource_data()
{
    QTest::addColumn<bool>("action");
    QTest::addColumn<bool>("accept");
    QTest::addColumn<bool>("imported");
    for (bool action : { false, true }) {
        for (bool accept : { false, true }) {
            for (bool imported : { false, true }) {
                const QString row = QStringLiteral("%1-%2-%3").arg(action ? "action" : "output")
                    .arg(accept ? "accept" : "cancel").arg(imported ? "model" : "project");
                QTest::newRow(qPrintable(row)) << action << accept << imported;
            }
        }
    }
}

void TestMethodScenarioEditor::doubleClickEditsSource()
{
    QFETCH(bool, action);
    QFETCH(bool, accept);
    QFETCH(bool, imported);
    MethodInfo info = method();
    if (imported)
        info.kind = MethodInfo::Kind::Model;
    info.response.actions = { { true, QStringLiteral("ns=1;s=A"), {} } };
    MethodPropertiesWidget widget;
    widget.setMethod(info);
    widget.resize(500, 1100);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    auto *table = widget.findChild<QTableWidget *>(action ? QStringLiteral("methodSetActionsTable")
                                                        : QStringLiteral("methodOutputsTable"));
    table->cellDoubleClicked(0, 0);
    QVERIFY(widget.findChildren<MethodSourceDialog *>().isEmpty());
    auto *scroll = widget.findChild<QScrollArea *>(QStringLiteral("methodScroll"));
    scroll->ensureWidgetVisible(table);
    const int column = action ? 2 : 3;
    const QPoint position = table->visualItemRect(table->item(0, column)).center();
    QTest::mouseClick(table->viewport(), Qt::LeftButton, Qt::NoModifier, position);
    _acceptSource = accept;
    _sourceDialogs = 0;
    QTimer::singleShot(0, this, &TestMethodScenarioEditor::configureSourceDialog);
    QTest::mouseDClick(table->viewport(), Qt::LeftButton, Qt::NoModifier, position);
    QCOMPARE(_sourceDialogs, 1);
    auto *apply = widget.findChild<QPushButton *>(QStringLiteral("methodApplyButton"));
    QCOMPARE(apply->isEnabled(), accept);
    if (!accept)
        return;
    QSignalSpy requested(&widget, &MethodPropertiesWidget::applyRequested);
    apply->click();
    QCOMPARE(requested.count(), 1);
    const MethodResponse response = requested.first().first().value<MethodInfo>().response;
    const MethodValueSource source = action ? response.actions.first().source : response.sources.first();
    QCOMPARE(source.kind, MethodValueSource::Kind::Input);
    QCOMPARE(source.inputIndex, 0);
}

/// \brief Completes the modal source editor during a simulated double-click.
void TestMethodScenarioEditor::configureSourceDialog()
{
    auto *dialog = qobject_cast<MethodSourceDialog *>(QApplication::activeModalWidget());
    QVERIFY(dialog);
    ++_sourceDialogs;
    if (!_acceptSource) {
        dialog->reject();
        return;
    }
    dialog->findChild<QComboBox *>(QStringLiteral("sourceCombo"))->setCurrentIndex(1);
    dialog->accept();
}

QTEST_MAIN(TestMethodScenarioEditor)
#include "test_methodscenarioeditor.moc"
