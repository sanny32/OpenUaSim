// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTest>
#include <QUaServer>

#include "appicons.h"
#include "server/typecatalog.h"

#include "addnodedialog.h"
#include "messagedialog.h"

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

///
/// \brief Presses OK and closes the warning it raises.
/// \return Warning text, or empty when no warning appeared.
///
QString pressOk(AddNodeDialog &dialog)
{
    dialog.setResult(QDialog::Rejected);
    child<QDialogButtonBox>(dialog, "buttonBox")->button(QDialogButtonBox::Ok)->click();
    MessageDialog *box = dialog.findChild<MessageDialog *>();
    if (!box)
        return {};
    const QString warning = child<QLabel>(*box, "textLabel")->text();
    delete box;
    return warning;
}

bool okAccepts(AddNodeDialog &dialog)
{
    return pressOk(dialog).isEmpty() && dialog.result() == QDialog::Accepted;
}

}

class TestAddNodeDialog : public QObject
{
    Q_OBJECT

private slots:
    void typeFamilyIcons();
    void typeCategoryIcons_data();
    void typeCategoryIcons();
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
    void allVariableTypesRespectConstraints();
    void importedFixedArraySelectsItsShape();
};


void TestAddNodeDialog::typeFamilyIcons()
{
    QUaServer server;
    auto types = TypeCatalog::types(server);
    const QHash<QString, QString> expected {
        { "BaseDataVariableType", "variable" }, { "AnalogItemType", "analog" },
        { "AnalogUnitType", "analog" }, { "ImageItemType", "matrix" },
        { "YArrayItemType", "array" }, { "XYArrayItemType", "array" },
        { "TwoStateDiscreteType", "discrete" }, { "MultiStateDiscreteType", "discrete" },
        { "MultiStateValueDiscreteType", "discrete" },
        { "MultiStateDictionaryEntryDiscreteType", "dictionary" },
    };
    AddNodeDialog dialog({}, {});
    dialog.setTypes(types, kNamespaces);
    child<QComboBox>(dialog, "kindCombo")->setCurrentIndex(2);
    const auto *combo = child<QComboBox>(dialog, "typeCombo");
    QSet<QString> checked;
    const auto standardTypes = types.values();
    for (const TypeInfo &info : standardTypes) {
        if (!expected.contains(info.browseName))
            continue;
        const QString icon = TypeCatalog::iconName(types, info.nodeId);
        QCOMPARE(icon, expected.value(info.browseName));
        QVERIFY(!AppIcons::icon(icon).pixmap(16, 16).isNull());
        if (icon == QLatin1String("analog")) {
            QVERIFY(AppIcons::icon(icon).pixmap(24, 24).toImage()
                    != AppIcons::icon(QStringLiteral("signal")).pixmap(24, 24).toImage());
        }
        const int index = combo->findText(info.browseName);
        QVERIFY(index >= 0);
        QVERIFY(!combo->itemIcon(index).isNull());
        const QString customId = "ns=2;s=Custom" + info.nodeId;
        types.insert(customId, type("Custom", customId, info.nodeId));
        QCOMPARE(TypeCatalog::iconName(types, customId), icon);
        types.remove(customId);
        checked.insert(info.browseName);
    }
    QCOMPARE(checked.size(), expected.size());
    QCOMPARE(TypeCatalog::iconName(types, "missing"), QStringLiteral("variable"));
    types.insert("cycle", type("Cycle", "cycle", "cycle"));
    QCOMPARE(TypeCatalog::iconName(types, "cycle"), QStringLiteral("variable"));
}

void TestAddNodeDialog::typeCategoryIcons_data()
{
    QTest::addColumn<QString>("category");
    QTest::addColumn<QString>("icon");
    QTest::addColumn<QString>("oldIcon");
    QTest::addColumn<QString>("rootId");
    QTest::addColumn<QStringList>("names");
    QTest::newRow("references")
        << "ReferenceType" << "reference-type" << "list-tree" << "i=31"
        << QStringList { "References", "HierarchicalReferences", "NonHierarchicalReferences" };
    QTest::newRow("data-types")
        << "DataType" << "data-type" << "hash" << "i=24"
        << QStringList { "BaseDataType", "Boolean", "DateTime", "Number", "Double", "String" };
}

void TestAddNodeDialog::typeCategoryIcons()
{
    QFETCH(QString, category);
    QFETCH(QString, icon);
    QFETCH(QString, oldIcon);
    QFETCH(QString, rootId);
    QFETCH(QStringList, names);
    QUaServer server;
    auto types = TypeCatalog::types(server);
    QSet<QString> checked;
    for (const TypeInfo &info : types) {
        if (info.category != category)
            continue;
        QCOMPARE(TypeCatalog::iconName(types, info.nodeId), icon);
        checked.insert(info.browseName);
    }
    for (const QString &name : names)
        QVERIFY2(checked.contains(name), qPrintable(name));

    TypeInfo custom = type("CustomType", "ns=2;s=CustomType", rootId);
    custom.category = category;
    types.insert(custom.nodeId, custom);
    QCOMPARE(TypeCatalog::iconName(types, custom.nodeId), icon);
    const QImage rendered = AppIcons::icon(icon).pixmap(24, 24).toImage();
    QVERIFY(!rendered.isNull());
    QVERIFY(rendered != AppIcons::icon(oldIcon).pixmap(24, 24).toImage());

    const QList<TreeNode> tree = TypeCatalog::tree(types, true);
    bool found = false;
    for (const TreeNode &root : tree.first().children) {
        if (root.id != rootId)
            continue;
        QCOMPARE(root.icon, icon);
        QVERIFY(!root.children.isEmpty());
        for (const TreeNode &child : root.children)
            QCOMPARE(child.icon, icon);
        found = true;
    }
    QVERIFY(found);
}

void TestAddNodeDialog::showsParentAndStartsInvalid()
{
    AddNodeDialog dialog(sampleTree(), kTank);
    QCOMPARE(child<QComboBox>(dialog, "parentCombo")->currentText(), QStringLiteral("Objects / Line / Tank"));
    QCOMPARE(dialog.parentId(), kTank);
    QVERIFY(child<QDialogButtonBox>(dialog, "buttonBox")->button(QDialogButtonBox::Ok)->isEnabled());
    QCOMPARE(dialog.problem(), QStringLiteral("Enter a browse name."));
    QCOMPARE(pressOk(dialog), QStringLiteral("Enter a browse name."));
    QCOMPARE(dialog.result(), int(QDialog::Rejected));
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
    QVERIFY(!okAccepts(dialog));

    auto *parents = child<QComboBox>(dialog, "parentCombo");
    parents->setCurrentIndex(parents->findData(kPump));
    QVERIFY(dialog.problem().isEmpty());
    QVERIFY(okAccepts(dialog));
    QCOMPARE(dialog.parentId(), kPump);
}

void TestAddNodeDialog::methodNeedsContainer()
{
    AddNodeDialog dialog(sampleTree(), QString());
    child<QComboBox>(dialog, "kindCombo")->setCurrentText(QStringLiteral("Method"));
    child<QLineEdit>(dialog, "browseNameEdit")->setText(QStringLiteral("Start"));
    QCOMPARE(dialog.problem(), QStringLiteral("Choose a folder or object for the method."));
    QVERIFY(!okAccepts(dialog));

    auto *parents = child<QComboBox>(dialog, "parentCombo");
    parents->setCurrentIndex(parents->findData(kPump));
    QVERIFY(okAccepts(dialog));
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
    QCOMPARE(pressOk(dialog), problem);
    QCOMPARE(dialog.result() == QDialog::Accepted, problem.isEmpty());
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
    QVERIFY(okAccepts(dialog));
}

///
/// \brief All offered standard variable types produce compatible defaults or an explicit blocking error.
///
void TestAddNodeDialog::allVariableTypesRespectConstraints()
{
    QUaServer server;
    const auto types = TypeCatalog::types(server);
    AddNodeDialog dialog({}, {});
    dialog.setTypes(types, server.namespaces());
    child<QComboBox>(dialog, "kindCombo")->setCurrentIndex(2);
    child<QLineEdit>(dialog, "browseNameEdit")->setText(QStringLiteral("Value"));
    auto *combo = child<QComboBox>(dialog, "typeCombo");
    int accepted = 0;
    int refused = 0;
    for (int index = 0; index < combo->count(); ++index) {
        combo->setCurrentIndex(index);
        const NodeDefinition node = dialog.node();
        const QString problem = TypeCatalog::variableProblem(server, node);
        QCOMPARE(dialog.problem(), problem);
        QCOMPARE(pressOk(dialog), problem);
        if (problem.isEmpty()) {
            QVERIFY(node.initialValue.isValid());
            ++accepted;
        } else {
            ++refused;
        }
    }
    QVERIFY(accepted > 5);
    QVERIFY(refused > 1);

    combo->setCurrentText(QStringLiteral("TwoStateDiscreteType"));
    QCOMPARE(itemTexts(*child<QComboBox>(dialog, "dataTypeCombo")), QStringList { QStringLiteral("Boolean") });
    child<QSpinBox>(dialog, "arrayLengthSpin")->setValue(0);
    QCOMPARE(dialog.node().arrayLength, 0);
    QVERIFY(child<QSpinBox>(dialog, "arrayLengthSpin")->isEnabled());
    QVERIFY(okAccepts(dialog));
    combo->setCurrentText(QStringLiteral("ImageItemType"));
    QVERIFY(!okAccepts(dialog));
    QVERIFY(dialog.problem().contains(QStringLiteral("multidimensional")));
    combo->setCurrentText(QStringLiteral("BaseDataVariableType"));
    QVERIFY(okAccepts(dialog));
    QVERIFY(child<QSpinBox>(dialog, "arrayLengthSpin")->isEnabled());
    child<QSpinBox>(dialog, "arrayLengthSpin")->setValue(3);
    QCOMPARE(dialog.node().initialValue.toList().size(), 3);
    child<QComboBox>(dialog, "kindCombo")->setCurrentIndex(1);
    QVERIFY(!child<QSpinBox>(dialog, "arrayLengthSpin")->isEnabled());
}

///
/// \brief Imported fixed arrays restrict both the element type and the length in the creation dialog.
///
void TestAddNodeDialog::importedFixedArraySelectsItsShape()
{
    auto types = sampleTypes();
    TypeInfo fixed = type("Fixed", "ns=2;i=5000", "i=63");
    fixed.dataTypeId = QStringLiteral("i=5");
    fixed.valueDataTypes = { QStringLiteral("UInt16") };
    fixed.valueRank = 1;
    fixed.arrayDimensions = { 3 };
    types.insert(fixed.nodeId, fixed);
    AddNodeDialog dialog({}, {});
    dialog.setTypes(types, kNamespaces);
    child<QComboBox>(dialog, "kindCombo")->setCurrentIndex(2);
    child<QLineEdit>(dialog, "browseNameEdit")->setText(QStringLiteral("Value"));
    child<QComboBox>(dialog, "typeCombo")->setCurrentText(QStringLiteral("Fixed"));
    const NodeDefinition node = dialog.node();
    QCOMPARE(node.typeDefinition, QStringLiteral("nsu=urn:pump;i=5000"));
    QCOMPARE(node.dataType, QStringLiteral("UInt16"));
    QCOMPARE(node.arrayLength, 3);
    QCOMPARE(child<QSpinBox>(dialog, "arrayLengthSpin")->text(), QStringLiteral("[3]"));
    QCOMPARE(node.initialValue.toList().size(), 3);
    QVERIFY(!child<QSpinBox>(dialog, "arrayLengthSpin")->isEnabled());
    QVERIFY(okAccepts(dialog));

    fixed.arrayDimensions = { quint32(kMaxArrayLength + 1) };
    types.insert(fixed.nodeId, fixed);
    dialog.setTypes(types, kNamespaces);
    child<QComboBox>(dialog, "typeCombo")->setCurrentText(QStringLiteral("Fixed"));
    QVERIFY(!okAccepts(dialog));
    QVERIFY(dialog.problem().contains(QString::number(kMaxArrayLength + 1)));

    fixed.valueRank = -1;
    fixed.arrayDimensions.clear();
    types.insert(fixed.nodeId, fixed);
    dialog.setTypes(types, kNamespaces);
    child<QComboBox>(dialog, "typeCombo")->setCurrentText(QStringLiteral("Fixed"));
    QCOMPARE(dialog.node().arrayLength, 0);
    QCOMPARE(child<QSpinBox>(dialog, "arrayLengthSpin")->text(), QStringLiteral("Scalar"));
    QVERIFY(!child<QSpinBox>(dialog, "arrayLengthSpin")->isEnabled());
    QVERIFY(okAccepts(dialog));
}

QTEST_MAIN(TestAddNodeDialog)

#include "test_addnodedialog.moc"
