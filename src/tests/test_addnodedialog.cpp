// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTest>

#include "addnodedialog.h"

namespace {

template <typename T>
T *child(const QWidget &parent, const char *name)
{
    T *found = parent.findChild<T *>(QLatin1String(name));
    if (!found)
        qFatal("Missing child %s", name);
    return found;
}

bool okEnabled(const AddNodeDialog &dialog)
{
    return child<QDialogButtonBox>(dialog, "buttonBox")->button(QDialogButtonBox::Ok)->isEnabled();
}

}

class TestAddNodeDialog : public QObject
{
    Q_OBJECT

private slots:
    void showsParentAndStartsInvalid();
    void validatesInput_data();
    void validatesInput();
    void describesVariable();
    void describesFolderWithoutVariableFields();
};

void TestAddNodeDialog::showsParentAndStartsInvalid()
{
    AddNodeDialog dialog(QStringLiteral("Tank_01"), {}, {});
    QCOMPARE(child<QLabel>(dialog, "parentValue")->text(), QStringLiteral("Tank_01"));
    QVERIFY(!okEnabled(dialog));
    QCOMPARE(dialog.problem(), QStringLiteral("Enter a browse name."));
}

void TestAddNodeDialog::validatesInput_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("nodeId");
    QTest::addColumn<QString>("problem");

    QTest::newRow("valid without id") << "Outlet" << "" << "";
    QTest::newRow("valid with id") << "Outlet" << "ns=1;s=Tank.Outlet" << "";
    QTest::newRow("valid numeric id") << "Outlet" << "i=7000" << "";
    QTest::newRow("blank name") << "   " << "" << "Enter a browse name.";
    QTest::newRow("sibling") << "Level" << "" << "A sibling is already named 'Level'.";
    QTest::newRow("malformed id") << "Outlet" << "Outlet" << "'Outlet' is not a node id such as ns=1;s=Name.";
    QTest::newRow("used id") << "Outlet" << "ns=1;s=Used" << "The node id 'ns=1;s=Used' is already used.";
}

void TestAddNodeDialog::validatesInput()
{
    QFETCH(QString, name);
    QFETCH(QString, nodeId);
    QFETCH(QString, problem);

    AddNodeDialog dialog(QStringLiteral("Tank_01"), { QStringLiteral("Level") }, { QStringLiteral("ns=1;s=Used") });
    child<QLineEdit>(dialog, "browseNameEdit")->setText(name);
    child<QLineEdit>(dialog, "nodeIdEdit")->setText(nodeId);
    QCOMPARE(dialog.problem(), problem);
    QCOMPARE(child<QLabel>(dialog, "problemLabel")->text(), problem);
    QCOMPARE(okEnabled(dialog), problem.isEmpty());
}

void TestAddNodeDialog::describesVariable()
{
    AddNodeDialog dialog(QStringLiteral("Tank_01"), {}, {});
    child<QComboBox>(dialog, "kindCombo")->setCurrentIndex(2);
    child<QLineEdit>(dialog, "browseNameEdit")->setText(QStringLiteral(" Outlet "));
    child<QComboBox>(dialog, "dataTypeCombo")->setCurrentText(QStringLiteral("UInt16"));
    child<QComboBox>(dialog, "accessCombo")->setCurrentText(QStringLiteral("Read"));

    const NodeDefinition node = dialog.node();
    QCOMPARE(node.kind, NodeDefinition::Kind::Variable);
    QCOMPARE(node.browseName, QStringLiteral("Outlet"));
    QVERIFY(node.nodeId.isEmpty());
    QCOMPARE(node.dataType, QStringLiteral("UInt16"));
    QCOMPARE(node.access, QStringLiteral("Read"));
    QCOMPARE(node.initialValue.typeId(), int(QMetaType::UShort));
    QCOMPARE(node.simulation.function, SimulationFunction::None);
}

void TestAddNodeDialog::describesFolderWithoutVariableFields()
{
    AddNodeDialog dialog(QStringLiteral("Objects"), {}, {});
    QVERIFY(!child<QComboBox>(dialog, "dataTypeCombo")->isEnabled());
    child<QComboBox>(dialog, "kindCombo")->setCurrentIndex(1);
    child<QLineEdit>(dialog, "browseNameEdit")->setText(QStringLiteral("Mixer"));
    QCOMPARE(dialog.node().kind, NodeDefinition::Kind::Object);
    QVERIFY(!dialog.node().initialValue.isValid());
    child<QComboBox>(dialog, "kindCombo")->setCurrentIndex(2);
    QVERIFY(child<QComboBox>(dialog, "dataTypeCombo")->isEnabled());
}

QTEST_MAIN(TestAddNodeDialog)

#include "test_addnodedialog.moc"
