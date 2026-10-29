// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file modelmethods.cpp
/// \brief Implements the mapping between the project answers of model methods and the server methods.
///

#include <QCoreApplication>

#include <QUaServer>

#include "importednodes.h"
#include "modelmethods.h"
#include "methodscenario.h"
#include "modelvariables.h"
#include "simulation/valuecast.h"

namespace ModelMethods {

///
/// \brief Returns the built-in data type the values of a data type are typed as.
/// \param server Server holding the data type.
/// \param typeId Data type node id.
/// \return Name such as "Double", "Int32" for an enumeration, or empty for a data type that cannot be typed as text.
///
QString valueType(const QUaServer &server, const QUaNodeId &typeId)
{
    if (server.isSubtypeOf(typeId, QUaNodeId(0, quint32(UA_NS0ID_ENUMERATION))))
        return QStringLiteral("Int32");
    // Subtypes such as Duration are typed as their built-in type.
    for (const QString &name : ValueCast::dataTypes()) {
        if (server.isSubtypeOf(typeId, QUaNodeId(ImportedNodes::dataTypeNodeId(name))))
            return name;
    }
    return {};
}

}

namespace {

///
/// \brief Translates a text of this file.
/// \param text Source text.
/// \return Translated text.
///
QString tr(const char *text)
{
    return QCoreApplication::translate("ModelMethods", text);
}

///
/// \brief Describes a method argument.
/// \param server Server holding the data type of the argument.
/// \param argument Argument as the method declares it.
/// \return Description with the name of the data type and the built-in type its text is parsed as.
///
ArgumentInfo argumentInfo(const QUaServer &server, const QUaMethodArgument &argument)
{
    ArgumentInfo info;
    info.name = argument.name;
    info.description = argument.description.text();
    info.array = argument.valueRank >= UA_VALUERANK_ONE_OR_MORE_DIMENSIONS;
    info.arrayDimensions = argument.arrayDimensions;
    const QString typeId = argument.dataType.toXmlString();
    QString typeName = ImportedNodes::dataTypeName(typeId);
    if (typeName == typeId && !server.browseName(argument.dataType).name().isEmpty())
        typeName = server.browseName(argument.dataType).name();
    info.dataType = info.array ? typeName + QStringLiteral("[]") : typeName;
    info.valueType = ModelMethods::valueType(server, argument.dataType);
    return info;
}

///
/// \brief Parses the text of an output argument.
/// \param output Output argument.
/// \param text Value text; an array takes elements separated by semicolons.
/// \return Value, or invalid when the text is no value of the argument.
///
QVariant outputValue(const ArgumentInfo &output, const QString &text)
{
    if (output.valueType.isEmpty())
        return {};
    if (!output.array)
        return ValueCast::fromText(text, output.valueType);
    QVariantList elements;
    for (const QString &element : text.split(QLatin1Char(';'))) {
        const QVariant value = ValueCast::fromText(element.trimmed(), output.valueType);
        if (!value.isValid())
            return {};
        elements.append(value);
    }
    return elements;
}

}

namespace ModelMethods {

///
/// \brief Describes a method with the arguments it declares.
/// \param server Server holding the method.
/// \param nodeId Server node id of the method.
/// \return Description of kind Model for a method of an imported model, otherwise Standard; the response is the
///         default one.
///
MethodInfo describe(const QUaServer &server, const QString &nodeId)
{
    const QUaNodeId id(nodeId);
    MethodInfo info;
    info.nodeId = nodeId;
    info.namespaces = server.namespaces();
    info.displayName = server.browseName(id).name();
    info.description = server.description(id).text();
    info.kind = id.namespaceIndex() >= ModelVariables::kFirstModelNamespace ? MethodInfo::Kind::Model
                                                                            : MethodInfo::Kind::Standard;
    for (const QUaMethodArgument &argument : server.inputArguments(id))
        info.inputs.append(argumentInfo(server, argument));
    for (const QUaMethodArgument &argument : server.outputArguments(id))
        info.outputs.append(argumentInfo(server, argument));
    return info;
}

///
/// \brief Parses the output texts of a response as values of the output arguments.
/// \param outputs Output arguments of the method.
/// \param response Response whose texts are parsed; texts beyond the arguments are ignored.
/// \param problem Receives which text is no value of its argument; may be null.
/// \return Values in argument order, invalid for an empty text; empty when a text is no value of its argument.
///
QVariantList outputValues(const QList<ArgumentInfo> &outputs, const MethodResponse &response, QString *problem)
{
    QVariantList values;
    for (qsizetype index = 0; index < outputs.size(); ++index) {
        const ArgumentInfo &output = outputs.at(index);
        const QString text = response.outputs.value(index);
        QVariant value;
        if (!text.trimmed().isEmpty()) {
            value = outputValue(output, text);
            if (!value.isValid()) {
                if (problem)
                    *problem = tr("'%1' is not a %2 value for '%3'").arg(text, output.dataType, output.name);
                return {};
            }
        }
        values.append(value);
    }
    return values;
}

///
/// \brief Finds the stored answer of a model method.
/// \param methods Answers kept by the project.
/// \param namespaceUri URI of the model namespace.
/// \param identifier Node id within the namespace, e.g. "i=5006".
/// \return Index into methods, or -1 when the project keeps none.
///
qsizetype indexOf(const QList<ModelMethod> &methods, const QString &namespaceUri, const QString &identifier)
{
    for (qsizetype index = 0; index < methods.size(); ++index) {
        if (methods.at(index).namespaceUri == namespaceUri && methods.at(index).identifier == identifier)
            return index;
    }
    return -1;
}

///
/// \brief Turns the stored answers into the answers of the served model methods.
/// \param methods Answers kept by the project.
/// \param server Server holding the models.
/// \param problems Receives why answers were skipped: an unloaded namespace, an unknown method or an output
///        text that is no value of its argument; may be null.
/// \return Answers keyed by the server node id; scenario dependencies are validated when called.
///
QHash<QString, MethodResponse> resolve(const QList<ModelMethod> &methods, QUaServer &server, QStringList *problems)
{
    const QStringList namespaces = server.namespaces();
    QHash<QString, MethodResponse> result;
    for (const ModelMethod &stored : methods) {
        const qsizetype namespaceIndex = namespaces.indexOf(stored.namespaceUri);
        QUaNodeId nodeId(stored.identifier);
        QString problem;
        if (namespaceIndex < ModelVariables::kFirstModelNamespace) {
            problem = tr("no imported model has the namespace");
        } else {
            nodeId.setNamespaceIndex(quint16(namespaceIndex));
            if (nodeId.isNull() || server.browseName(nodeId).name().isEmpty())
                problem = tr("the model has no such method");
            // Scenario dependencies are checked on calls so missing nodes return an error instead of default Good.
            else if (stored.response.sources.isEmpty() && stored.response.actions.isEmpty()) {
                MethodInfo method = describe(server, nodeId.toXmlString());
                method.response = stored.response;
                problem = MethodScenario::problem(server, method);
            }
        }
        if (!problem.isEmpty()) {
            if (problems)
                problems->append(QStringLiteral("%1;%2: %3").arg(stored.namespaceUri, stored.identifier, problem));
            continue;
        }
        result.insert(nodeId.toXmlString(), stored.response);
    }
    return result;
}

}
