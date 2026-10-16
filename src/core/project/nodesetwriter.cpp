// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file nodesetwriter.cpp
/// \brief Implements the export of project nodes as an OPC UA NodeSet2 document.
///

#include <QCoreApplication>
#include <QDateTime>
#include <QRegularExpression>
#include <QSaveFile>
#include <QXmlStreamWriter>

#include "nodesetwriter.h"
#include "simulation/valuecast.h"

namespace {

const QString kNodeSetNamespace = QStringLiteral("http://opcfoundation.org/UA/2011/03/UANodeSet.xsd");
const QString kTypesNamespace = QStringLiteral("http://opcfoundation.org/UA/2008/02/Types.xsd");
const QString kObjectsFolder = QStringLiteral("i=85");
constexpr int kCurrentRead = 0x01;
constexpr int kCurrentWrite = 0x02;

///
/// \brief Returns the namespace index of a node id in XML notation.
/// \param nodeId Node id such as "ns=1;s=Name".
/// \return Index, 0 when the id has no "ns=" prefix.
///
int namespaceIndex(const QString &nodeId)
{
    static const QRegularExpression prefix(QStringLiteral("^ns=(\\d+);"));
    const QRegularExpressionMatch match = prefix.match(nodeId);
    return match.hasMatch() ? match.captured(1).toInt() : 0;
}

///
/// \brief Returns the OPC UA AccessLevel of an access text.
/// \param access "Read", "Write" or "Read/Write".
/// \return CurrentRead and CurrentWrite bits.
///
int accessLevel(const QString &access)
{
    int level = 0;
    if (access != QLatin1String("Write"))
        level |= kCurrentRead;
    if (access != QLatin1String("Read"))
        level |= kCurrentWrite;
    return level;
}

///
/// \brief Formats a value as the text of a NodeSet2 scalar element.
/// \param value Value of the variable's data type.
/// \return Text such as "true", "12.5" or an ISO 8601 time.
///
QString valueText(const QVariant &value)
{
    switch (value.typeId()) {
    case QMetaType::Bool:      return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    case QMetaType::QDateTime: return value.toDateTime().toUTC().toString(Qt::ISODateWithMs);
    case QMetaType::Float:     return QString::number(value.toFloat(), 'g', 9);
    case QMetaType::Double:    return QString::number(value.toDouble(), 'g', 17);
    default:                   return value.toString();
    }
}

///
/// \brief Writes the alias table of the built-in data types and references used.
/// \param xml Writer positioned inside UANodeSet.
///
void writeAliases(QXmlStreamWriter &xml)
{
    xml.writeStartElement(QStringLiteral("Aliases"));
    const QStringList types = ValueCast::dataTypes();
    for (int index = 0; index < types.size(); ++index) {
        xml.writeStartElement(QStringLiteral("Alias"));
        xml.writeAttribute(QStringLiteral("Alias"), types.at(index));
        // Boolean..DateTime are the built-in types i=1..i=13 in the order dataTypes() lists them.
        xml.writeCharacters(QStringLiteral("i=%1").arg(index + 1));
        xml.writeEndElement();
    }
    const QList<QPair<QString, int>> references = {
        { QStringLiteral("Organizes"), 35 },
        { QStringLiteral("HasTypeDefinition"), 40 },
        { QStringLiteral("HasComponent"), 47 },
    };
    for (const auto &[alias, id] : references) {
        xml.writeStartElement(QStringLiteral("Alias"));
        xml.writeAttribute(QStringLiteral("Alias"), alias);
        xml.writeCharacters(QStringLiteral("i=%1").arg(id));
        xml.writeEndElement();
    }
    xml.writeEndElement();
}

///
/// \brief Writes one reference.
/// \param xml Writer positioned inside References.
/// \param type Reference type alias.
/// \param target Target node id.
/// \param forward Whether the reference points from this node to the target.
///
void writeReference(QXmlStreamWriter &xml, const QString &type, const QString &target, bool forward)
{
    xml.writeStartElement(QStringLiteral("Reference"));
    xml.writeAttribute(QStringLiteral("ReferenceType"), type);
    if (!forward)
        xml.writeAttribute(QStringLiteral("IsForward"), QStringLiteral("false"));
    xml.writeCharacters(target);
    xml.writeEndElement();
}

///
/// \brief Writes a node and, after it, its children.
/// \param xml Writer positioned inside UANodeSet.
/// \param node Node to write.
/// \param parent Parent node id.
/// \param parentIsFolder Whether the parent organizes rather than contains its children.
///
void writeNode(QXmlStreamWriter &xml, const NodeDefinition &node, const QString &parent, bool parentIsFolder)
{
    const bool variable = node.kind == NodeDefinition::Kind::Variable;
    xml.writeStartElement(variable ? QStringLiteral("UAVariable") : QStringLiteral("UAObject"));
    xml.writeAttribute(QStringLiteral("NodeId"), node.nodeId);
    xml.writeAttribute(QStringLiteral("BrowseName"),
                       QStringLiteral("%1:%2").arg(namespaceIndex(node.nodeId)).arg(node.browseName));
    xml.writeAttribute(QStringLiteral("ParentNodeId"), parent);
    if (variable) {
        const QString level = QString::number(accessLevel(node.access));
        xml.writeAttribute(QStringLiteral("DataType"), node.dataType);
        xml.writeAttribute(QStringLiteral("AccessLevel"), level);
        xml.writeAttribute(QStringLiteral("UserAccessLevel"), level);
    }
    xml.writeTextElement(QStringLiteral("DisplayName"), node.browseName);

    xml.writeStartElement(QStringLiteral("References"));
    writeReference(xml, parentIsFolder ? QStringLiteral("Organizes") : QStringLiteral("HasComponent"), parent, false);
    const QString typeDefinition = variable ? QStringLiteral("i=63")
                                   : node.kind == NodeDefinition::Kind::Folder ? QStringLiteral("i=61")
                                                                              : QStringLiteral("i=58");
    writeReference(xml, QStringLiteral("HasTypeDefinition"), typeDefinition, true);
    xml.writeEndElement();

    if (variable) {
        xml.writeStartElement(QStringLiteral("Value"));
        xml.writeTextElement(kTypesNamespace, node.dataType,
                             valueText(ValueCast::convert(node.initialValue, node.dataType)));
        xml.writeEndElement();
    }
    xml.writeEndElement();

    for (const NodeDefinition &child : node.children)
        writeNode(xml, child, node.nodeId, node.kind == NodeDefinition::Kind::Folder);
}

}

namespace NodeSetWriter {

///
/// \brief Returns the file name filter of NodeSet2 files for file dialogs.
/// \return Filter such as "NodeSet2 XML (*.xml)".
///
QString fileFilter()
{
    return QCoreApplication::translate("NodeSetWriter", "NodeSet2 XML (*.xml)");
}

///
/// \brief Writes project nodes as a NodeSet2 document.
/// \param nodes Node tree with node ids; top-level nodes are organized by the Objects folder.
/// \param namespaceUris URIs of namespace indexes 1, 2, ... used by the node ids.
/// \return UTF-8 XML with folders and objects as UAObject and variables as UAVariable carrying
///         their data type, access level and initial value.
///
QByteArray write(const QList<NodeDefinition> &nodes, const QStringList &namespaceUris)
{
    QByteArray document;
    QXmlStreamWriter xml(&document);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeDefaultNamespace(kNodeSetNamespace);
    xml.writeNamespace(kTypesNamespace, QStringLiteral("uax"));
    xml.writeStartElement(kNodeSetNamespace, QStringLiteral("UANodeSet"));

    xml.writeStartElement(QStringLiteral("NamespaceUris"));
    for (const QString &uri : namespaceUris)
        xml.writeTextElement(QStringLiteral("Uri"), uri);
    xml.writeEndElement();

    writeAliases(xml);
    for (const NodeDefinition &node : nodes)
        writeNode(xml, node, kObjectsFolder, true);

    xml.writeEndElement();
    xml.writeEndDocument();
    return document;
}

///
/// \brief Writes project nodes to a NodeSet2 file, replacing it atomically.
/// \param nodes Node tree with node ids.
/// \param namespaceUris URIs of namespace indexes 1, 2, ... used by the node ids.
/// \param fileName Target path.
/// \param error Receives the reason on failure; may be null.
/// \return True when the file was written.
///
bool save(const QList<NodeDefinition> &nodes, const QStringList &namespaceUris, const QString &fileName,
          QString *error)
{
    QSaveFile file(fileName);
    if (file.open(QIODevice::WriteOnly) && file.write(write(nodes, namespaceUris)) >= 0 && file.commit())
        return true;
    if (error)
        *error = file.errorString();
    return false;
}

}
