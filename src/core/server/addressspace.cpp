// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspace.cpp
/// \brief Implements the project nodes built under the Objects folder of a QUaServer.
///

#include <QRegularExpression>

#include <QUaExclusiveLevelAlarm>
#include <QUaOffNormalAlarm>
#include <QUaProperty>
#include <QUaServer>

#include <algorithm>

#include "addressspace.h"
#include "importednodes.h"
#include "project/alarmrules.h"
#include "project/portablenodeid.h"
#include "simulation/valuecast.h"
#include "variableproperties.h"

namespace {

constexpr quint16 kApplicationNamespace = 1;

///
/// \brief Returns the key under which a declared child of a node keeps its node id.
/// \param parentId Server node id of the parent.
/// \param browseName Browse name of the child, without namespace.
/// \return Key for declaredChildId().
///
QString childKey(const QUaNodeId &parentId, const QString &browseName)
{
    return parentId.toXmlString() + QLatin1Char('/') + browseName;
}

///
/// \brief Collects the node ids of the declared children a node keeps settings for, at every depth.
/// \param node Instance definition.
/// \param ids Receives the node ids keyed by childKey().
///
void collectDeclaredIds(const NodeDefinition &node, QHash<QString, QString> &ids)
{
    for (const NodeDefinition &child : node.children) {
        if (!child.declared)
            continue;
        ids.insert(childKey(QUaNodeId(node.nodeId), child.browseName), child.nodeId);
        collectDeclaredIds(child, ids);
    }
}

///
/// \brief Returns the node id of a child that instantiating a type adds.
/// \param ids Node ids of children with settings, keyed by childKey(); copies of an instance keep theirs this way.
/// \param parentId Server node id of the parent.
/// \param browseName Browse name of the child.
/// \return The stored node id, or one derived from the parent, e.g. "ns=1;s=Line.Motor.Speed".
///
QUaNodeId declaredChildId(const QHash<QString, QString> &ids, const QUaNodeId &parentId,
                          const QUaQualifiedName &browseName)
{
    const QString stored = ids.value(childKey(parentId, browseName.name()));
    if (!stored.isEmpty())
        return QUaNodeId(stored);
    static const QRegularExpression namespacePart(QStringLiteral("^ns=\\d+;(s=)?"));
    const QString parent = parentId.toXmlString().remove(namespacePart);
    return QUaNodeId(QStringLiteral("ns=%1;s=%2.%3").arg(kApplicationNamespace).arg(parent, browseName.name()));
}

///
/// \brief Finds the child a type declares by its browse name.
/// \param parent Instance or declared node.
/// \param browseName Browse name without namespace.
/// \return Child whose browse name is outside the application namespace, or null.
///
QUaNode *declaredChild(const QUaNode &parent, const QString &browseName)
{
    const QList<QUaNode *> children = parent.browseChildren();
    for (QUaNode *child : children) {
        const QUaQualifiedName name = child->browseName();
        if (name.namespaceIndex() != kApplicationNamespace && name.name() == browseName)
            return child;
    }
    return nullptr;
}

///
/// \brief Adds the data item properties that the types of an instance and of the variables below it declare.
/// \param server Server holding the instance.
/// \param node Built instance or a node below it.
///
void addDeclaredProperties(const QUaServer &server, QUaNode &node)
{
    if (auto *variable = qobject_cast<QUaBaseDataVariable *>(&node))
        VariableProperties::addDeclared(server, *variable);
    const QList<QUaNode *> children = node.browseChildren();
    for (QUaNode *child : children) {
        if (!qobject_cast<QUaProperty *>(child))
            addDeclaredProperties(server, *child);
    }
}

///
/// \brief Applies an access level text to a variable.
/// \param variable Variable to restrict.
/// \param access "Read", "Write" or "Read/Write".
///
void applyAccess(QUaBaseDataVariable *variable, const QString &access)
{
    variable->setReadAccess(access != QLatin1String("Write"));
    variable->setWriteAccess(access != QLatin1String("Read"));
}

///
/// \brief Starts or stops recording the values of a variable and lets clients read the history while it records.
/// \param variable Variable to change.
/// \param historizing Whether to record.
///
void applyHistorizing(QUaBaseDataVariable *variable, bool historizing)
{
    variable->setHistorizing(historizing);
    variable->setReadHistoryAccess(historizing);
}

static_assert(quint32(VariableInfo::Quality::GoodLocalOverride) == UA_STATUSCODE_GOODLOCALOVERRIDE);
static_assert(quint32(VariableInfo::Quality::UncertainNoCommunicationLastUsableValue)
              == UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE);
static_assert(quint32(VariableInfo::Quality::UncertainSubNormal) == UA_STATUSCODE_UNCERTAINSUBNORMAL);
static_assert(quint32(VariableInfo::Quality::BadConfigurationError) == UA_STATUSCODE_BADCONFIGURATIONERROR);
static_assert(quint32(VariableInfo::Quality::BadOutOfService) == UA_STATUSCODE_BADOUTOFSERVICE);
static_assert(quint32(VariableInfo::Quality::BadNoCommunication) == UA_STATUSCODE_BADNOCOMMUNICATION);
static_assert(quint32(VariableInfo::Quality::BadWaitingForInitialData) == UA_STATUSCODE_BADWAITINGFORINITIALDATA);

///
/// \brief Returns the status code a quality is reported with.
/// \param quality Quality to report.
/// \return The status code the quality stands for.
///
QUaStatusCode statusCodeOf(VariableInfo::Quality quality)
{
    return QUaStatusCode(UA_StatusCode(quality));
}

///
/// \brief Sets a variable's value with the Qt type of its data type.
/// \param variable Variable to update.
/// \param value Value to convert; a scalar fills every element of an array.
/// \param shape Data type, array length and quality of the variable.
///
void setTypedValue(QUaBaseDataVariable *variable, const QVariant &value, const AddressSpace::VariableShape &shape)
{
    const QVariant typed = shape.arrayLength > 0
        ? QVariant(ValueCast::toArray(value, shape.dataType, shape.arrayLength))
        : ValueCast::convert(value, shape.dataType);
    variable->setValue(typed, statusCodeOf(shape.quality), QDateTime(), QDateTime(), ValueCast::metaType(shape.dataType));
}

}

///
/// \brief Creates an empty address space on a server.
/// \param server Server that receives the nodes; must outlive this object.
/// \param parent Owning QObject.
///
AddressSpace::AddressSpace(QUaServer *server, QObject *parent)
    : QObject(parent)
    , _server(server)
{
}

///
/// \brief Removes the built nodes from the server.
///
AddressSpace::~AddressSpace()
{
    clear();
}

///
/// \brief Replaces the built nodes with a node tree.
/// \param nodes Tree to build under the Objects folder; nodes need node ids.
/// \return Problems with nodes that were skipped together with their children: a missing,
///         malformed or already used node id, an unknown namespace, a duplicated browse
///         name or an unsupported data type.
///
QStringList AddressSpace::build(const QList<NodeDefinition> &nodes)
{
    clear();
    QStringList problems;
    for (const NodeDefinition &node : nodes) {
        if (QUaNode *root = buildNode(_server->objectsFolder(), node, node.browseName, problems))
            _roots.append(root);
    }
    return problems;
}

///
/// \brief Takes over variables of imported models, so they are set, simulated and alarmed like built ones.
/// \param variables Variable definitions with the server node ids of model variables; the access, quality,
///        historizing, alarm and a valid initial value are applied. Variables taken over before are skipped.
/// \return Problems with variables that are not served.
///
QStringList AddressSpace::attach(const QList<NodeDefinition> &variables)
{
    QStringList problems;
    for (const NodeDefinition &node : variables) {
        if (hasVariable(node.nodeId))
            continue;
        auto *variable = _server->nodeById<QUaBaseDataVariable>(QUaNodeId(node.nodeId));
        if (!variable) {
            problems.append(QStringLiteral("%1: %2").arg(node.nodeId, tr("the server has no such variable")));
            continue;
        }
        registerVariable(variable, node, node.nodeId, problems);
        if (node.alarm.enabled)
            addAlarm(qobject_cast<QUaNode *>(variable->parent()), *variable, node, node.nodeId, problems);
    }
    return problems;
}

///
/// \brief Removes every built node from the server and releases the attached model variables.
///
void AddressSpace::clear()
{
    // Alarms of top-level variables live in the Objects folder, which the roots do not own.
    for (const QPointer<QUaAlarmCondition> &alarm : std::as_const(_alarms)) {
        if (alarm) {
            alarm->disconnect(this);
            delete alarm.data();
        }
    }
    _alarms.clear();
    _alarmIds.clear();
    for (const QPointer<QUaBaseDataVariable> &variable : std::as_const(_variables)) {
        if (variable)
            variable->disconnect(this);
    }
    for (const QPointer<QUaNode> &root : std::as_const(_roots))
        delete root.data();
    _roots.clear();
    _variables.clear();
    _shapes.clear();
    _variableIds.clear();
    _nodeCount = 0;
}

///
/// \brief Removes the built nodes and builds later ones on another server.
/// \param server Server that receives the nodes; must outlive this object or the next call.
///
void AddressSpace::setServer(QUaServer *server)
{
    clear();
    _server = server;
}

///
/// \brief Returns the number of built nodes.
/// \return Folders, objects and variables built by the last build().
///
int AddressSpace::nodeCount() const
{
    return _nodeCount;
}

///
/// \brief Tells whether a variable was built or attached.
/// \param nodeId Variable node id.
/// \return True when the variable exists.
///
bool AddressSpace::hasVariable(const QString &nodeId) const
{
    return !_variables.value(nodeId).isNull();
}

///
/// \brief Returns the current value of a variable.
/// \param nodeId Variable node id.
/// \return Value, or an invalid variant for an unknown variable.
///
QVariant AddressSpace::value(const QString &nodeId) const
{
    const QUaBaseDataVariable *variable = _variables.value(nodeId);
    return variable ? variable->value() : QVariant();
}

///
/// \brief Returns the quality of a variable's value.
/// \param nodeId Variable node id.
/// \return The quality of the status code; Bad for an unknown variable.
///
VariableInfo::Quality AddressSpace::quality(const QString &nodeId) const
{
    const QUaBaseDataVariable *variable = _variables.value(nodeId);
    return variable ? qualityOf(*variable) : VariableInfo::Quality::Bad;
}

///
/// \brief Returns the quality of any server variable's value.
/// \param variable Variable to inspect.
/// \return The quality of its status code; the generic one of its severity for an unlisted code.
///
VariableInfo::Quality AddressSpace::qualityOf(const QUaBaseVariable &variable)
{
    return VariableInfo::qualityOf(UA_StatusCode(variable.statusCode()));
}

///
/// \brief Sets the value of a variable from the server side, keeping its data type.
/// \param nodeId Variable node id.
/// \param value New value, converted to the variable's data type; a scalar fills every element of an array.
/// \return False for an unknown variable.
///
bool AddressSpace::setValue(const QString &nodeId, const QVariant &value)
{
    QUaBaseDataVariable *variable = _variables.value(nodeId);
    if (!variable)
        return false;
    setTypedValue(variable, value, _shapes.value(nodeId));
    return true;
}

///
/// \brief Changes who may read and write a variable.
/// \param nodeId Variable node id.
/// \param access "Read", "Write" or "Read/Write".
/// \return False for an unknown variable.
///
bool AddressSpace::setAccess(const QString &nodeId, const QString &access)
{
    QUaBaseDataVariable *variable = _variables.value(nodeId);
    if (!variable)
        return false;
    applyAccess(variable, access);
    return true;
}

///
/// \brief Changes the quality reported with the current and every later value of a variable.
/// \param nodeId Variable node id.
/// \param quality Quality to report.
/// \return False for an unknown variable.
///
bool AddressSpace::setQuality(const QString &nodeId, VariableInfo::Quality quality)
{
    QUaBaseDataVariable *variable = _variables.value(nodeId);
    if (!variable)
        return false;
    _shapes[nodeId].quality = quality;
    variable->setStatusCode(statusCodeOf(quality));
    return true;
}

///
/// \brief Starts or stops recording the value history of a variable; recorded values are kept.
/// \param nodeId Variable node id.
/// \param historizing Whether to record and let clients read the history.
/// \return False for an unknown variable.
///
bool AddressSpace::setHistorizing(const QString &nodeId, bool historizing)
{
    QUaBaseDataVariable *variable = _variables.value(nodeId);
    if (!variable)
        return false;
    applyHistorizing(variable, historizing);
    return true;
}

///
/// \brief Builds one node and, when that succeeds, its children.
/// \param parent Server node receiving the new node.
/// \param node Definition to build.
/// \param path Slash-separated browse path for problem messages.
/// \param problems Receives the reason the node was skipped.
/// \return Created node, or null when it was skipped.
///
QUaNode *AddressSpace::buildNode(QUaNode *parent, const NodeDefinition &node, const QString &path,
                                 QStringList &problems)
{
    const QString problem = checkNode(parent, node);
    if (!problem.isEmpty()) {
        problems.append(QStringLiteral("%1: %2").arg(path, problem));
        return nullptr;
    }

    const QUaQualifiedName browseName(kApplicationNamespace, node.browseName);
    const QUaNodeId nodeId(node.nodeId);
    QUaNode *created = nullptr;
    switch (node.kind) {
    case NodeDefinition::Kind::Folder:
        created = node.typeDefinition.isEmpty() ? parent->addFolderObject(browseName, nodeId)
                                                : addInstance(parent, node, path, problems);
        break;
    case NodeDefinition::Kind::Object:
        created = node.typeDefinition.isEmpty() ? parent->addBaseObject(browseName, nodeId)
                                                : addInstance(parent, node, path, problems);
        break;
    case NodeDefinition::Kind::Variable: {
        QUaBaseDataVariable *variable = addVariable(parent, node, path, problems);
        if (variable && node.alarm.enabled)
            addAlarm(parent, *variable, node, path, problems);
        created = variable;
        break;
    }
    case NodeDefinition::Kind::Method:
        problems.append(QStringLiteral("%1: %2").arg(path, tr("a method belongs to a folder or object")));
        return nullptr;
    }
    if (!created)
        return nullptr;
    ++_nodeCount;
    for (const NodeDefinition &child : node.children) {
        const QString childPath = path + QLatin1Char('/') + child.browseName;
        if (child.declared)
            attachDeclared(*created, child, childPath, problems);
        else if (child.kind == NodeDefinition::Kind::Method)
            addMethod(created, child, node.children, childPath, problems);
        else
            buildNode(created, child, childPath, problems);
    }
    return created;
}

///
/// \brief Adds the alarm of a variable next to it: an exclusive level alarm for numbers, an off-normal
///        alarm for Booleans.
/// \param parent Folder or object holding the variable.
/// \param variable Variable whose value drives the alarm.
/// \param node Variable definition with enabled, validated alarm settings.
/// \param path Slash-separated browse path for problem messages.
/// \param problems Receives the reason the alarm was skipped.
///
void AddressSpace::addAlarm(QUaNode *parent, QUaBaseDataVariable &variable, const NodeDefinition &node,
                            const QString &path, QStringList &problems)
{
    auto *owner = qobject_cast<QUaBaseObject *>(parent);
    const QUaQualifiedName name(kApplicationNamespace, node.browseName + QStringLiteral("Alarm"));
    if (!owner || owner->hasChild(name)) {
        problems.append(QStringLiteral("%1: %2").arg(path, tr("alarm name '%1' is used by a sibling").arg(name.name())));
        return;
    }
    owner->setSubscribeToEvents(true);

    QUaAlarmCondition *alarm = nullptr;
    if (AlarmRules::isOffNormal(node.dataType)) {
        auto *offNormal = owner->addChild<QUaOffNormalAlarm>(name);
        offNormal->setNormalValue(node.alarm.normalValue);
        connect(offNormal, &QUaAlarmCondition::activated, this, &AddressSpace::handleAlarmChanged);
        connect(offNormal, &QUaAlarmCondition::deactivated, this, &AddressSpace::handleAlarmChanged);
        alarm = offNormal;
    } else {
        auto *level = owner->addChild<QUaExclusiveLevelAlarm>(name);
        level->setHighHighLimitRequired(node.alarm.highHighEnabled);
        level->setHighLimitRequired(node.alarm.highEnabled);
        level->setLowLimitRequired(node.alarm.lowEnabled);
        level->setLowLowLimitRequired(node.alarm.lowLowEnabled);
        level->setHighHighLimit(node.alarm.highHigh);
        level->setHighLimit(node.alarm.high);
        level->setLowLimit(node.alarm.low);
        level->setLowLowLimit(node.alarm.lowLow);
        // Also announces moves between limits, e.g. from High to HighHigh, while the alarm stays active.
        connect(level, &QUaExclusiveLimitAlarm::exclusiveLimitStateChanged, this, &AddressSpace::handleAlarmChanged);
        alarm = level;
    }
    alarm->setInputNode(&variable);
    alarm->setConditionName(node.browseName);
    alarm->setSeverity(quint16(node.alarm.severity));
    alarm->setEnabled(true);
    _alarms.insert(node.nodeId, alarm);
    _alarmIds.insert(alarm, node.nodeId);
    // The alarm evaluates value changes only; replaying the current value covers an initial value beyond a limit.
    emit variable.valueChanged(variable.value(), false);
}

///
/// \brief Returns the active state of a variable's alarm.
/// \param nodeId Variable node id.
/// \return Violated limit such as "High" for a level alarm, "Active" for an off-normal alarm, or empty
///         while the alarm is inactive or the variable has none.
///
QString AddressSpace::alarmState(const QString &nodeId) const
{
    const QUaAlarmCondition *alarm = _alarms.value(nodeId);
    if (!alarm)
        return {};
    // A level alarm changes its limit state before its active state, so the limit state decides.
    if (const auto *level = qobject_cast<const QUaExclusiveLimitAlarm *>(alarm)) {
        const QUaExclusiveLimitState state = level->exclusiveLimitState();
        return state == QUa::ExclusiveLimitState::None ? QString() : state.toString();
    }
    return alarm->active() ? tr("Active") : QString();
}

///
/// \brief Returns the type of a built or attached variable.
/// \param nodeId Variable node id.
/// \return Browse name of its type definition, e.g. "AnalogItemType"; empty for an unknown variable.
///
QString AddressSpace::typeName(const QString &nodeId) const
{
    const QUaBaseDataVariable *variable = _variables.value(nodeId);
    return variable ? variable->typeDefinitionBrowseName().name() : QString();
}

///
/// \brief Returns the properties of a built or attached variable.
/// \param nodeId Variable node id.
/// \return Properties as VariableProperties::describe() lists them; none for an unknown variable.
///
QList<PropertyInfo> AddressSpace::properties(const QString &nodeId) const
{
    const QUaBaseDataVariable *variable = _variables.value(nodeId);
    return variable ? VariableProperties::describe(*_server, *variable) : QList<PropertyInfo>();
}

///
/// \brief Announces that an alarm became active or inactive.
///
void AddressSpace::handleAlarmChanged()
{
    const QString nodeId = _alarmIds.value(sender());
    if (!nodeId.isEmpty())
        emit alarmChanged(nodeId, alarmState(nodeId));
}

///
/// \brief Adds a method that acts on the variables next to it.
/// \param parent Folder or object receiving the method.
/// \param method Method definition.
/// \param siblings Definitions of the parent's children, whose variables the method acts on.
/// \param path Slash-separated browse path for problem messages.
/// \param problems Receives the reason the method was skipped.
///
void AddressSpace::addMethod(QUaNode *parent, const NodeDefinition &method, const QList<NodeDefinition> &siblings,
                             const QString &path, QStringList &problems)
{
    const QString problem = checkNode(parent, method);
    if (!problem.isEmpty()) {
        problems.append(QStringLiteral("%1: %2").arg(path, problem));
        return;
    }
    auto *object = qobject_cast<QUaBaseObject *>(parent);
    QList<NodeDefinition> variables;
    for (const NodeDefinition &sibling : siblings) {
        if (sibling.kind == NodeDefinition::Kind::Variable)
            variables.append(sibling);
    }
    const QUaQualifiedName browseName(kApplicationNamespace, method.browseName);
    const QString methodId = method.nodeId;
    switch (method.method) {
    case NodeDefinition::MethodAction::Reset:
        object->addMethod(browseName, [this, methodId, variables]() { return resetVariables(methodId, variables); },
                          QUaNodeId(methodId));
        break;
    case NodeDefinition::MethodAction::SetValue:
        object->addMethod(browseName,
                          [this, methodId, variables](QString name, QString value) {
                              return setVariableFromText(methodId, variables, name, value);
                          },
                          QUaNodeId(methodId));
        break;
    }
    ++_nodeCount;
}

///
/// \brief Runs a Reset method: sets each variable to its initial value.
/// \param methodId Node id of the called method.
/// \param variables Variables the method acts on.
/// \return Result text for the client, e.g. "Reset 3 variables".
///
QString AddressSpace::resetVariables(const QString &methodId, const QList<NodeDefinition> &variables)
{
    QHash<QString, QVariant> values;
    for (const NodeDefinition &variable : variables) {
        if (setValue(variable.nodeId, variable.initialValue))
            values.insert(variable.nodeId, value(variable.nodeId));
    }
    const QString result = tr("Reset %n variables", nullptr, int(values.size()));
    emit valuesSetByMethod(values);
    emit methodCalled(methodId, result);
    return result;
}

///
/// \brief Runs a SetValue method: parses a value for one variable and sets it.
/// \param methodId Node id of the called method.
/// \param variables Variables the method may set.
/// \param name Browse name of the variable to set.
/// \param text Value as text of the variable's data type; it fills every element of an array.
/// \return Result text for the client: what was set, or why nothing was.
///
QString AddressSpace::setVariableFromText(const QString &methodId, const QList<NodeDefinition> &variables,
                                          const QString &name, const QString &text)
{
    const auto target = std::find_if(variables.cbegin(), variables.cend(),
                                     [&name](const NodeDefinition &variable) { return variable.browseName == name; });
    QString result;
    if (target == variables.cend()) {
        result = tr("There is no variable '%1'").arg(name);
    } else if (const QVariant parsed = ValueCast::fromText(text, target->dataType); !parsed.isValid()) {
        result = tr("'%1' is not a %2 value").arg(text, target->dataType);
    } else if (setValue(target->nodeId, parsed)) {
        result = tr("Set %1 to %2").arg(name, text);
        emit valuesSetByMethod({ { target->nodeId, value(target->nodeId) } });
    } else {
        result = tr("The variable '%1' is not built").arg(name);
    }
    emit methodCalled(methodId, result);
    return result;
}

///
/// \brief Finds what would make QUaServer refuse a node.
/// \param parent Server node that would receive it.
/// \param node Definition to check.
/// \return Problem text, or empty when the node can be built.
///
QString AddressSpace::checkNode(QUaNode *parent, const NodeDefinition &node) const
{
    if (node.browseName.isEmpty())
        return tr("browse name is empty");
    if (node.nodeId.isEmpty())
        return tr("node id is empty");
    const QUaNodeId nodeId(node.nodeId);
    if (nodeId.isNull())
        return tr("node id '%1' is malformed").arg(node.nodeId);
    if (nodeId.namespaceIndex() >= _server->namespaces().size())
        return tr("namespace %1 of node id '%2' is not registered").arg(nodeId.namespaceIndex()).arg(node.nodeId);
    if (_server->isNodeIdUsed(nodeId))
        return tr("node id '%1' is already used").arg(node.nodeId);
    if (parent->hasChild(QUaQualifiedName(kApplicationNamespace, node.browseName)))
        return tr("browse name '%1' is used by a sibling").arg(node.browseName);
    if (node.kind == NodeDefinition::Kind::Variable && !ValueCast::isSupported(node.dataType))
        return tr("unsupported data type '%1'").arg(node.dataType);
    if (!node.typeDefinition.isEmpty() && PortableNodeId::toServer(node.typeDefinition, _server->namespaces()).isEmpty())
        return tr("type '%1' is not loaded").arg(node.typeDefinition);
    return {};
}

///
/// \brief Adds an instance of the type of a node; the children the type declares get node ids below the node's.
/// \param parent Server node receiving the instance.
/// \param node Folder, object or variable definition with a loaded type, already checked.
/// \param path Slash-separated browse path for problem messages.
/// \param problems Receives why the instance was not added.
/// \return Instance, or null when the type cannot be instantiated or makes no node of the definition's kind.
///
QUaNode *AddressSpace::addInstance(QUaNode *parent, const NodeDefinition &node, const QString &path,
                                   QStringList &problems)
{
    QHash<QString, QString> childIds;
    collectDeclaredIds(node, childIds);
    _server->setChildNodeIdCallback([&childIds](const QUaNodeId &parentId, const QUaQualifiedName &browseName) {
        return declaredChildId(childIds, parentId, browseName);
    });
    const QUaNodeId typeId(PortableNodeId::toServer(node.typeDefinition, _server->namespaces()));
    QUaNode *instance = parent->addInstance(QUaQualifiedName(kApplicationNamespace, node.browseName), typeId,
                                            QUaNodeId(node.nodeId));
    _server->setChildNodeIdCallback(QUaChildNodeIdCallback());

    const bool fits = node.kind == NodeDefinition::Kind::Variable
        ? qobject_cast<QUaBaseDataVariable *>(instance) != nullptr
        : qobject_cast<QUaBaseObject *>(instance) != nullptr;
    if (!fits) {
        delete instance;
        problems.append(QStringLiteral("%1: %2").arg(path, tr("type '%1' cannot be instantiated as this node")
                                                               .arg(node.typeDefinition)));
        return nullptr;
    }
    addDeclaredProperties(*_server, *instance);
    return instance;
}

///
/// \brief Applies the settings a project keeps for a node that the type of an ancestor declares.
/// \param parent Built instance or declared node holding the node.
/// \param node Declared folder, object or variable; a container passes the settings on to its declared children.
/// \param path Slash-separated browse path for problem messages.
/// \param problems Receives why settings were skipped: the type declares no such node or the simulation cannot
///        drive it.
///
void AddressSpace::attachDeclared(QUaNode &parent, const NodeDefinition &node, const QString &path,
                                  QStringList &problems)
{
    QUaNode *declared = declaredChild(parent, node.browseName);
    if (!declared || declared->nodeId() != QUaNodeId(node.nodeId)) {
        problems.append(QStringLiteral("%1: %2").arg(path, tr("the type declares no such node")));
        return;
    }
    if (node.kind != NodeDefinition::Kind::Variable) {
        for (const NodeDefinition &child : node.children) {
            if (child.declared)
                attachDeclared(*declared, child, path + QLatin1Char('/') + child.browseName, problems);
        }
        return;
    }
    auto *variable = qobject_cast<QUaBaseDataVariable *>(declared);
    if (!variable || !ValueCast::isSupported(node.dataType)) {
        problems.append(QStringLiteral("%1: %2").arg(path, tr("the simulation cannot drive this node")));
        return;
    }
    registerVariable(variable, node, path, problems);
    if (node.alarm.enabled)
        addAlarm(&parent, *variable, node, path, problems);
}

///
/// \brief Creates a variable, of its type when it has one, with its data type, access level and initial value.
/// \param parent Server node receiving the variable.
/// \param node Variable definition, already checked.
/// \param path Slash-separated browse path for problem messages.
/// \param problems Receives why the variable was not created.
/// \return Created variable, or null when the server refused it or its type takes no values of its data type.
///
QUaBaseDataVariable *AddressSpace::addVariable(QUaNode *parent, const NodeDefinition &node, const QString &path,
                                               QStringList &problems)
{
    QUaBaseDataVariable *variable = nullptr;
    if (node.typeDefinition.isEmpty()) {
        variable = parent->addBaseDataVariable(QUaQualifiedName(kApplicationNamespace, node.browseName),
                                               QUaNodeId(node.nodeId));
    } else {
        variable = qobject_cast<QUaBaseDataVariable *>(addInstance(parent, node, path, problems));
        const QUaNodeId dataTypeId(ImportedNodes::dataTypeNodeId(node.dataType));
        if (variable && !_server->isSubtypeOf(dataTypeId, QUaNodeId(variable->dataTypeNodeId()))) {
            problems.append(QStringLiteral("%1: %2").arg(path, tr("type '%1' takes no %2 values")
                                                                   .arg(node.typeDefinition, node.dataType)));
            delete variable;
            return nullptr;
        }
    }
    if (!variable)
        return nullptr;
    if (node.arrayLength > 0) {
        variable->setValueRank(1);
        variable->setArrayDimensions({ quint32(node.arrayLength) });
    }
    registerVariable(variable, node, path, problems);
    return variable;
}

///
/// \brief Applies the historizing, value, quality, access and properties of a definition to a variable and serves
///        it by node id.
/// \param variable Built or attached variable.
/// \param node Variable definition; an invalid initial value keeps the current value.
/// \param path Slash-separated browse path for problem messages.
/// \param problems Receives why property values were skipped.
///
void AddressSpace::registerVariable(QUaBaseDataVariable *variable, const NodeDefinition &node, const QString &path,
                                    QStringList &problems)
{
    const QStringList propertyProblems = VariableProperties::apply(*_server, *variable, node.properties);
    for (const QString &problem : propertyProblems)
        problems.append(QStringLiteral("%1: %2").arg(path, problem));
    // Historizing first, so the history starts with the initial value.
    applyHistorizing(variable, node.historizing);
    const VariableShape shape { node.dataType, node.arrayLength, node.quality };
    setTypedValue(variable, node.initialValue.isValid() ? node.initialValue : variable->value(), shape);
    applyAccess(variable, node.access);
    _variables.insert(node.nodeId, variable);
    _shapes.insert(node.nodeId, shape);
    _variableIds.insert(variable, node.nodeId);
    connect(variable, &QUaBaseVariable::valueChanged, this, &AddressSpace::handleValueChanged);
}

///
/// \brief Reports values written by clients.
/// \param value New value.
/// \param networkChange True when a client wrote it.
///
void AddressSpace::handleValueChanged(const QVariant &value, const bool &networkChange)
{
    if (networkChange)
        emit valueWrittenByClient(_variableIds.value(sender()), value);
}
