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

NodeDefinition node(NodeDefinition::Kind kind, const QString &name, const QString &nodeId,
                    const QList<NodeDefinition> &children = {})
{
    NodeDefinition result;
    result.kind = kind;
    result.browseName = name;
    result.nodeId = nodeId;
    result.children = children;
    return result;
}

const QString kTank = QStringLiteral("ns=1;s=Line.Tank");
const QString kPump = QStringLiteral("ns=1;s=Pump");

QList<NodeDefinition> sampleTree()
{
    using Kind = NodeDefinition::Kind;
    const NodeDefinition tank = node(Kind::Object, QStringLiteral("Tank"), kTank,
                                     { node(Kind::Variable, QStringLiteral("Level"), QStringLiteral("ns=1;s=Used")),
                                       node(Kind::Method, QStringLiteral("Reset"), QStringLiteral("ns=1;s=Line.Tank.Reset")) });
    return { node(Kind::Folder, QStringLiteral("Line"), QStringLiteral("ns=1;s=Line"), { tank }),
             node(Kind::Object, QStringLiteral("Pump"), kPump) };
}

TypeInfo type(const QString &name, const QString &nodeId, const QString &baseTypeId, bool isAbstract = false)
{
    TypeInfo info;
    info.browseName = name;
    info.nodeId = nodeId;
    info.baseTypeId = baseTypeId;
    info.isAbstract = isAbstract;
    return info;
}

QHash<QString, TypeInfo> sampleTypes()
{
    const QList<TypeInfo> types {
        type(QStringLiteral("BaseObjectType"), QStringLiteral("i=58"), QString()),
        type(QStringLiteral("FolderType"), QStringLiteral("i=61"), QStringLiteral("i=58")),
        type(QStringLiteral("BaseVariableType"), QStringLiteral("i=62"), QString(), true),
        type(QStringLiteral("BaseDataVariableType"), QStringLiteral("i=63"), QStringLiteral("i=62")),
        type(QStringLiteral("PumpType"), QStringLiteral("ns=2;i=1000"), QStringLiteral("i=58")),
        type(QStringLiteral("MachineType"), QStringLiteral("ns=2;i=2000"), QStringLiteral("i=58"), true),
    };
    QHash<QString, TypeInfo> result;
    for (const TypeInfo &info : types)
        result.insert(info.nodeId, info);
    return result;
}

const QStringList kNamespaces { QStringLiteral("http://opcfoundation.org/UA/"), QStringLiteral("urn:app"),
                                QStringLiteral("urn:pump") };

QStringList itemTexts(const QComboBox &combo)
{
    QStringList texts;
    for (int i = 0; i < combo.count(); ++i)
        texts.append(combo.itemText(i));
    return texts;
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
    void offersOnlyContainersAsParents();
    void unknownParentSelectsTopLevel();
    void siblingCheckFollowsParent();
    void methodNeedsContainer();
    void validatesInput_data();
    void validatesInput();
    void describesVariable();
    void describesFolderWithoutVariableFields();
    void describesMethod();
    void offersTypesOfTheKind();
    void describesTypedObject();
    void withoutTypesKeepsStandardType();
};

void TestAddNodeDialog::showsParentAndStartsInvalid()
{
    AddNodeDialog dialog(sampleTree(), kTank);
    QCOMPARE(child<QComboBox>(dialog, "parentCombo")->currentText(), QStringLiteral("Objects / Line / Tank"));
    QCOMPARE(dialog.parentId(), kTank);
    QVERIFY(!okEnabled(dialog));
    QCOMPARE(dialog.problem(), QStringLiteral("Enter a browse name."));
}

void TestAddNodeDialog::offersOnlyContainersAsParents()
{
    AddNodeDialog dialog(sampleTree(), QString());
    auto *parents = child<QComboBox>(dialog, "parentCombo");
    QStringList labels;
    for (int i = 0; i < parents->count(); ++i)
        labels.append(parents->itemText(i));
    QCOMPARE(labels, QStringList({ QStringLiteral("Objects"), QStringLiteral("Objects / Line"),
                                   QStringLiteral("Objects / Line / Tank"), QStringLiteral("Objects / Pump") }));
    QVERIFY(parents->itemData(0).toString().isEmpty());
    QCOMPARE(parents->itemData(3).toString(), kPump);
}

void TestAddNodeDialog::unknownParentSelectsTopLevel()
{
    AddNodeDialog dialog(sampleTree(), QStringLiteral("ns=1;s=Line.Tank.Reset"));
    QCOMPARE(child<QComboBox>(dialog, "parentCombo")->currentIndex(), 0);
    QVERIFY(dialog.parentId().isEmpty());
}

void TestAddNodeDialog::siblingCheckFollowsParent()
{
    AddNodeDialog dialog(sampleTree(), kTank);
    child<QLineEdit>(dialog, "browseNameEdit")->setText(QStringLiteral("Level"));
    QVERIFY(!okEnabled(dialog));

    auto *parents = child<QComboBox>(dialog, "parentCombo");
    parents->setCurrentIndex(parents->findData(kPump));
    QVERIFY(dialog.problem().isEmpty());
    QVERIFY(okEnabled(dialog));
    QCOMPARE(dialog.parentId(), kPump);
}

void TestAddNodeDialog::methodNeedsContainer()
{
    AddNodeDialog dialog(sampleTree(), QString());
    child<QComboBox>(dialog, "kindCombo")->setCurrentText(QStringLiteral("Method"));
    child<QLineEdit>(dialog, "browseNameEdit")->setText(QStringLiteral("Start"));
    QCOMPARE(dialog.problem(), QStringLiteral("Choose a folder or object for the method."));
    QVERIFY(!okEnabled(dialog));

    auto *parents = child<QComboBox>(dialog, "parentCombo");
    parents->setCurrentIndex(parents->findData(kPump));
    QVERIFY(okEnabled(dialog));
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

    AddNodeDialog dialog(sampleTree(), kTank);
    child<QLineEdit>(dialog, "browseNameEdit")->setText(name);
    child<QLineEdit>(dialog, "nodeIdEdit")->setText(nodeId);
    QCOMPARE(dialog.problem(), problem);
    QCOMPARE(child<QLabel>(dialog, "problemLabel")->text(), problem);
    QCOMPARE(okEnabled(dialog), problem.isEmpty());
}

void TestAddNodeDialog::describesVariable()
{
    AddNodeDialog dialog(sampleTree(), kTank);
    child<QComboBox>(dialog, "kindCombo")->setCurrentIndex(2);
    child<QLineEdit>(dialog, "browseNameEdit")->setText(QStringLiteral(" Outlet "));
    child<QComboBox>(dialog, "dataTypeCombo")->setCurrentText(QStringLiteral("UInt16"));
    child<QComboBox>(dialog, "accessCombo")->setCurrentText(QStringLiteral("Read"));
    auto *icons = child<QComboBox>(dialog, "iconCombo");
    icons->setCurrentIndex(icons->findData(QStringLiteral("thermometer")));

    const NodeDefinition node = dialog.node();
    QCOMPARE(node.kind, NodeDefinition::Kind::Variable);
    QCOMPARE(node.browseName, QStringLiteral("Outlet"));
    QVERIFY(node.nodeId.isEmpty());
    QCOMPARE(node.dataType, QStringLiteral("UInt16"));
    QCOMPARE(node.access, QStringLiteral("Read"));
    QCOMPARE(node.icon, QStringLiteral("thermometer"));
    QCOMPARE(node.initialValue.typeId(), int(QMetaType::UShort));
    QCOMPARE(node.simulation.function, SimulationFunction::None);
}

void TestAddNodeDialog::describesFolderWithoutVariableFields()
{
    AddNodeDialog dialog(sampleTree(), QString());
    QVERIFY(!child<QComboBox>(dialog, "dataTypeCombo")->isEnabled());
    QVERIFY(!child<QComboBox>(dialog, "iconCombo")->isEnabled());
    child<QComboBox>(dialog, "kindCombo")->setCurrentIndex(1);
    child<QLineEdit>(dialog, "browseNameEdit")->setText(QStringLiteral("Mixer"));
    QCOMPARE(dialog.node().kind, NodeDefinition::Kind::Object);
    QVERIFY(!dialog.node().initialValue.isValid());
    QVERIFY(dialog.node().icon.isEmpty());
    child<QComboBox>(dialog, "kindCombo")->setCurrentIndex(2);
    QVERIFY(child<QComboBox>(dialog, "dataTypeCombo")->isEnabled());
    QVERIFY(child<QComboBox>(dialog, "iconCombo")->isEnabled());
    QVERIFY(!child<QComboBox>(dialog, "actionCombo")->isEnabled());
}

void TestAddNodeDialog::describesMethod()
{
    AddNodeDialog dialog(sampleTree(), kPump);
    child<QComboBox>(dialog, "kindCombo")->setCurrentText(QStringLiteral("Method"));
    child<QLineEdit>(dialog, "browseNameEdit")->setText(QStringLiteral("SetValue"));
    auto *action = child<QComboBox>(dialog, "actionCombo");
    QVERIFY(action->isEnabled());
    QVERIFY(!child<QComboBox>(dialog, "dataTypeCombo")->isEnabled());
    action->setCurrentIndex(1);

    const NodeDefinition node = dialog.node();
    QCOMPARE(node.kind, NodeDefinition::Kind::Method);
    QCOMPARE(node.method, NodeDefinition::MethodAction::SetValue);
}

void TestAddNodeDialog::offersTypesOfTheKind()
{
    AddNodeDialog dialog(sampleTree(), QString());
    dialog.setTypes(sampleTypes(), kNamespaces);
    auto *types = child<QComboBox>(dialog, "typeCombo");
    QCOMPARE(itemTexts(*types), QStringList({ QStringLiteral("FolderType") }));
    QVERIFY(!types->isEnabled());

    child<QComboBox>(dialog, "kindCombo")->setCurrentIndex(1);
    QCOMPARE(itemTexts(*types), QStringList({ QStringLiteral("BaseObjectType"), QStringLiteral("PumpType") }));
    QVERIFY(types->isEnabled());

    child<QComboBox>(dialog, "kindCombo")->setCurrentText(QStringLiteral("Method"));
    QCOMPARE(types->count(), 0);
    QVERIFY(!types->isEnabled());
}

void TestAddNodeDialog::describesTypedObject()
{
    AddNodeDialog dialog(sampleTree(), kPump);
    dialog.setTypes(sampleTypes(), kNamespaces);
    child<QComboBox>(dialog, "kindCombo")->setCurrentIndex(1);
    child<QLineEdit>(dialog, "browseNameEdit")->setText(QStringLiteral("Motor"));
    QVERIFY(dialog.node().typeDefinition.isEmpty());

    child<QComboBox>(dialog, "typeCombo")->setCurrentText(QStringLiteral("PumpType"));
    QCOMPARE(dialog.node().typeDefinition, QStringLiteral("nsu=urn:pump;i=1000"));
}

void TestAddNodeDialog::withoutTypesKeepsStandardType()
{
    AddNodeDialog dialog(sampleTree(), kPump);
    child<QComboBox>(dialog, "kindCombo")->setCurrentIndex(1);
    child<QLineEdit>(dialog, "browseNameEdit")->setText(QStringLiteral("Motor"));
    QVERIFY(!child<QComboBox>(dialog, "typeCombo")->isEnabled());
    QVERIFY(dialog.node().typeDefinition.isEmpty());
    QVERIFY(okEnabled(dialog));
}

QTEST_MAIN(TestAddNodeDialog)

#include "test_addnodedialog.moc"
