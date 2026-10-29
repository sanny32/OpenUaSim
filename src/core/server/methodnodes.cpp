// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

/// \file methodnodes.cpp
/// \brief Creates editable method signatures through the NodeSet importer.

#include <QBuffer>
#include <QCryptographicHash>
#include <QXmlStreamWriter>
#include <QUaServer>

#include "importednodes.h"
#include "methodnodes.h"
#include "methodscenario.h"

namespace {

/// \brief Writes an OPC UA reference into the generated method NodeSet.
void reference(QXmlStreamWriter &xml, const QString &type, const QString &target, bool forward = true)
{
    xml.writeStartElement(QStringLiteral("Reference"));
    xml.writeAttribute(QStringLiteral("ReferenceType"), type);
    if (!forward)
        xml.writeAttribute(QStringLiteral("IsForward"), QStringLiteral("false"));
    xml.writeCharacters(target);
    xml.writeEndElement();
}

/// \brief Writes an Argument property whose data types stay in namespace zero.
void argumentProperty(QXmlStreamWriter &xml, const QString &methodId, const QString &propertyId,
                        const QString &name, const QList<ArgumentInfo> &arguments)
{
    xml.writeStartElement(QStringLiteral("UAVariable"));
    xml.writeAttribute(QStringLiteral("NodeId"), propertyId);
    xml.writeAttribute(QStringLiteral("BrowseName"), name);
    xml.writeAttribute(QStringLiteral("DataType"), QStringLiteral("i=296"));
    xml.writeAttribute(QStringLiteral("ValueRank"), QStringLiteral("1"));
    xml.writeAttribute(QStringLiteral("ArrayDimensions"), QString::number(arguments.size()));
    xml.writeStartElement(QStringLiteral("References"));
    reference(xml, QStringLiteral("i=46"), methodId, false);
    reference(xml, QStringLiteral("i=40"), QStringLiteral("i=68"));
    xml.writeEndElement();
    xml.writeStartElement(QStringLiteral("Value"));
    xml.writeStartElement(QStringLiteral("uax:ListOfExtensionObject"));
    for (const ArgumentInfo &argument : arguments) {
        xml.writeStartElement(QStringLiteral("uax:ExtensionObject"));
        xml.writeStartElement(QStringLiteral("uax:TypeId"));
        xml.writeTextElement(QStringLiteral("uax:Identifier"), QStringLiteral("i=297"));
        xml.writeEndElement();
        xml.writeStartElement(QStringLiteral("uax:Body"));
        xml.writeStartElement(QStringLiteral("uax:Argument"));
        xml.writeTextElement(QStringLiteral("uax:Name"), argument.name);
        xml.writeStartElement(QStringLiteral("uax:DataType"));
        xml.writeTextElement(QStringLiteral("uax:Identifier"), ImportedNodes::dataTypeNodeId(argument.valueType));
        xml.writeEndElement();
        xml.writeTextElement(QStringLiteral("uax:ValueRank"), argument.array ? QStringLiteral("1") : QStringLiteral("-1"));
        xml.writeEmptyElement(QStringLiteral("uax:ArrayDimensions"));
        xml.writeStartElement(QStringLiteral("uax:Description"));
        xml.writeTextElement(QStringLiteral("uax:Text"), argument.description);
        xml.writeEndElement();
        xml.writeEndElement();
        xml.writeEndElement();
        xml.writeEndElement();
    }
    xml.writeEndElement();
    xml.writeEndElement();
    xml.writeEndElement();
}

}

namespace MethodNodes {

///
/// \brief Adds a project method with its configured signature using the NodeSet importer.
/// \param server Server receiving the method.
/// \param parentId Existing parent object node id.
/// \param method Definition of a scenario method.
/// \return Import error, or empty on success.
///
QString add(QUaServer &server, const QString &parentId, const NodeDefinition &method)
{
    for (const QList<ArgumentInfo> &arguments : { method.inputs, method.outputs }) {
        const QString error = MethodScenario::signatureProblem(arguments);
        if (!error.isEmpty())
            return error;
    }
    QByteArray data;
    QXmlStreamWriter xml(&data);
    xml.writeStartDocument();
    xml.writeStartElement(QStringLiteral("UANodeSet"));
    xml.writeDefaultNamespace(QStringLiteral("http://opcfoundation.org/UA/2011/03/UANodeSet.xsd"));
    xml.writeNamespace(QStringLiteral("http://opcfoundation.org/UA/2008/02/Types.xsd"), QStringLiteral("uax"));
    xml.writeStartElement(QStringLiteral("NamespaceUris"));
    const QStringList namespaces = server.namespaces();
    for (qsizetype index = 1; index < namespaces.size(); ++index)
        xml.writeTextElement(QStringLiteral("Uri"), namespaces.at(index));
    xml.writeEndElement();
    xml.writeStartElement(QStringLiteral("UAMethod"));
    xml.writeAttribute(QStringLiteral("NodeId"), method.nodeId);
    xml.writeAttribute(QStringLiteral("BrowseName"), QStringLiteral("1:") + method.browseName);
    xml.writeAttribute(QStringLiteral("ParentNodeId"), parentId);
    xml.writeTextElement(QStringLiteral("DisplayName"), method.browseName);
    xml.writeStartElement(QStringLiteral("References"));
    reference(xml, QStringLiteral("i=47"), parentId, false);
    xml.writeEndElement();
    xml.writeEndElement();
    const QString prefix = QStringLiteral("ns=1;s=__arguments.%1.")
        .arg(QString::fromLatin1(QCryptographicHash::hash(method.nodeId.toUtf8(), QCryptographicHash::Sha256).toHex()));
    int suffix = 0;
    QString inputId;
    QString outputId;
    do {
        inputId = prefix + QString::number(suffix++) + QStringLiteral(".in");
        outputId = inputId + QStringLiteral(".out");
    } while (server.isNodeIdUsed(QUaNodeId(inputId)) || server.isNodeIdUsed(QUaNodeId(outputId)));
    if (!method.inputs.isEmpty())
        argumentProperty(xml, method.nodeId, inputId, QStringLiteral("InputArguments"), method.inputs);
    if (!method.outputs.isEmpty())
        argumentProperty(xml, method.nodeId, outputId, QStringLiteral("OutputArguments"), method.outputs);
    xml.writeEndElement();
    xml.writeEndDocument();
    QBuffer buffer(&data);
    buffer.open(QIODevice::ReadOnly);
    const QUaNodeSetResult loaded = server.loadNodeSet(&buffer);
    if (!loaded.isOk())
        return loaded.errorString;
    return loaded.warnings.join(QLatin1Char('\n'));
}

}
