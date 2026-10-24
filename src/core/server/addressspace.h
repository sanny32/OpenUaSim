// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspace.h
/// \brief Declares the project nodes built under the Objects folder of a QUaServer.
///

#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QVariant>

#include "data/projectconfig.h"
#include "simulation/valuesink.h"

class QUaAlarmCondition;
class QUaBaseDataVariable;
class QUaBaseVariable;
class QUaNode;
class QUaServer;

///
/// \brief Builds project nodes in a QUaServer and gives access to their variables by node id.
///
class AddressSpace : public QObject, public ValueSink
{
    Q_OBJECT

public:
    ///
    /// \brief How the values of a built variable are typed and reported.
    ///
    struct VariableShape
    {
        /// \brief Built-in data type name of the value or its elements.
        QString dataType;
        /// \brief Number of array elements; 0 for a scalar.
        int arrayLength = 0;
        /// \brief Quality reported with every value.
        VariableInfo::Quality quality = VariableInfo::Quality::Good;
    };

    ///
    /// \brief Creates an empty address space on a server.
    /// \param server Server that receives the nodes; must outlive this object.
    /// \param parent Owning QObject.
    ///
    explicit AddressSpace(QUaServer *server, QObject *parent = nullptr);

    ///
    /// \brief Removes the built nodes from the server.
    ///
    ~AddressSpace() override;

    ///
    /// \brief Replaces the built nodes with a node tree and observes project and imported variable writes.
    /// \param nodes Tree to build under the Objects folder; nodes need node ids.
    /// \return Problems with nodes that were skipped together with their children: a missing,
    ///         malformed or already used node id, an unknown namespace, a duplicated browse
    ///         name or an unsupported data type.
    ///
    QStringList build(const QList<NodeDefinition> &nodes);

    ///
    /// \brief Takes over variables of imported models, so they are set, simulated and alarmed like built ones.
    /// \param variables Variable definitions with the server node ids of model variables; the access, quality,
    ///        historizing, alarm and a valid initial value are applied. Variables taken over before are skipped.
    /// \return Problems with variables that are not served.
    ///
    QStringList attach(const QList<NodeDefinition> &variables);

    ///
    /// \brief Removes every built node from the server and releases the attached model variables.
    ///
    void clear();

    ///
    /// \brief Removes the built nodes and builds later ones on another server.
    /// \param server Server that receives the nodes; must outlive this object or the next call.
    ///
    void setServer(QUaServer *server);

    ///
    /// \brief Returns the number of built nodes.
    /// \return Folders, objects and variables built by the last build().
    ///
    int nodeCount() const;

    ///
    /// \brief Tells whether a variable was built or attached.
    /// \param nodeId Variable node id.
    /// \return True when the variable exists.
    ///
    bool hasVariable(const QString &nodeId) const;

    ///
    /// \brief Returns the current value of a variable.
    /// \param nodeId Variable node id.
    /// \return Value, or an invalid variant for an unknown variable.
    ///
    QVariant value(const QString &nodeId) const;

    ///
    /// \brief Returns the quality of a variable's value.
    /// \param nodeId Variable node id.
    /// \return The quality of the status code; Bad for an unknown variable.
    ///
    VariableInfo::Quality quality(const QString &nodeId) const;

    ///
    /// \brief Returns the quality of any server variable's value.
    /// \param variable Variable to inspect.
    /// \return The quality of its status code; the generic one of its severity for an unlisted code.
    ///
    static VariableInfo::Quality qualityOf(const QUaBaseVariable &variable);

    ///
    /// \brief Sets the value of a variable from the server side, keeping its data type.
    /// \param nodeId Variable node id.
    /// \param value New value, converted to the variable's data type; a scalar fills every element of an array.
    /// \return False for an unknown variable.
    ///
    bool setValue(const QString &nodeId, const QVariant &value) override;

    ///
    /// \brief Changes who may read and write a variable.
    /// \param nodeId Variable node id.
    /// \param access "Read", "Write" or "Read/Write".
    /// \return False for an unknown variable.
    ///
    bool setAccess(const QString &nodeId, const QString &access);

    ///
    /// \brief Changes the quality reported with the current and every later value of a variable.
    /// \param nodeId Variable node id.
    /// \param quality Quality to report.
    /// \return False for an unknown variable.
    ///
    bool setQuality(const QString &nodeId, VariableInfo::Quality quality);

    ///
    /// \brief Starts or stops recording the value history of a variable; recorded values are kept.
    /// \param nodeId Variable node id.
    /// \param historizing Whether to record and let clients read the history.
    /// \return False for an unknown variable.
    ///
    bool setHistorizing(const QString &nodeId, bool historizing);

    ///
    /// \brief Returns the active state of a variable's alarm.
    /// \param nodeId Variable node id.
    /// \return Violated limit such as "High" for a level alarm, "Active" for an off-normal alarm, or empty
    ///         while the alarm is inactive or the variable has none.
    ///
    QString alarmState(const QString &nodeId) const;

    ///
    /// \brief Returns the type of a built or attached variable.
    /// \param nodeId Variable node id.
    /// \return Browse name of its type definition, e.g. "AnalogItemType"; empty for an unknown variable.
    ///
    QString typeName(const QString &nodeId) const;

    ///
    /// \brief Returns the properties of a built or attached variable.
    /// \param nodeId Variable node id.
    /// \return Properties as VariableProperties::describe() lists them; none for an unknown variable.
    ///
    QList<PropertyInfo> properties(const QString &nodeId) const;

signals:
    ///
    /// \brief Announces a value written by an OPC UA client.
    /// \param nodeId Variable node id.
    /// \param value Written value.
    ///
    void valueWrittenByClient(const QString &nodeId, const QVariant &value);

    ///
    /// \brief Announces a change in the live quality of a served variable.
    /// \param nodeId Variable whose current quality changed.
    /// \param quality Quality reported by the server.
    ///
    void qualityChanged(const QString &nodeId, VariableInfo::Quality quality);

    ///
    /// \brief Announces values a method call set.
    /// \param values New values keyed by node id.
    ///
    void valuesSetByMethod(const QHash<QString, QVariant> &values);

    ///
    /// \brief Announces a method call.
    /// \param methodId Node id of the called method.
    /// \param result Result text returned to the client.
    ///
    void methodCalled(const QString &methodId, const QString &result);

    ///
    /// \brief Announces that the alarm of a variable became active or inactive.
    /// \param nodeId Variable node id.
    /// \param state New state as alarmState() returns it.
    ///
    void alarmChanged(const QString &nodeId, const QString &state);

private:
    QUaNode *buildNode(QUaNode *parent, const NodeDefinition &node, const QString &path, QStringList &problems);
    void addAlarm(QUaNode *parent, QUaBaseDataVariable &variable, const NodeDefinition &node, const QString &path,
                  QStringList &problems);
    void handleAlarmChanged();
    void addMethod(QUaNode *parent, const NodeDefinition &method, const QList<NodeDefinition> &siblings,
                   const QString &path, QStringList &problems);
    QString resetVariables(const QString &methodId, const QList<NodeDefinition> &variables);
    QString setVariableFromText(const QString &methodId, const QList<NodeDefinition> &variables,
                                const QString &name, const QString &text);
    QString checkNode(QUaNode *parent, const NodeDefinition &node) const;
    QUaNode *addInstance(QUaNode *parent, const NodeDefinition &node, const QString &path, QStringList &problems);
    void attachDeclared(QUaNode &parent, const NodeDefinition &node, const QString &path, QStringList &problems);
    QUaBaseDataVariable *addVariable(QUaNode *parent, const NodeDefinition &node, const QString &path,
                                     QStringList &problems);
    void registerVariable(QUaBaseDataVariable *variable, const NodeDefinition &node, const QString &path,
                          QStringList &problems);
    void observeVariable(QUaBaseDataVariable *variable, const QString &nodeId);
    void handleValueChanged(const QVariant &value, const bool &networkChange);

    QUaServer *_server;
    QList<QPointer<QUaNode>> _roots;
    QHash<QString, QPointer<QUaBaseDataVariable>> _variables;
    QList<QPointer<QUaBaseDataVariable>> _observedVariables;
    QHash<QString, VariableShape> _shapes;
    QHash<QString, VariableInfo::Quality> _qualities;
    QHash<const QObject *, QString> _variableIds;
    QHash<QString, QPointer<QUaAlarmCondition>> _alarms;
    QHash<const QObject *, QString> _alarmIds;
    int _nodeCount = 0;
};
