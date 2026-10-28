// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file modelmethods.h
/// \brief Declares the mapping between the project answers of model methods and the server methods.
///

#pragma once

#include <QHash>
#include <QList>
#include <QStringList>
#include <QVariantList>

#include "data/methodinfo.h"

class QUaServer;

namespace ModelMethods {

///
/// \brief Describes a method with the arguments it declares.
/// \param server Server holding the method.
/// \param nodeId Server node id of the method.
/// \return Description of kind Model for a method of an imported model, otherwise Standard; the response is the
///         default one.
///
MethodInfo describe(const QUaServer &server, const QString &nodeId);

///
/// \brief Parses the output texts of a response as values of the output arguments.
/// \param outputs Output arguments of the method.
/// \param response Response whose texts are parsed; texts beyond the arguments are ignored.
/// \param problem Receives which text is no value of its argument; may be null.
/// \return Values in argument order, invalid for an empty text; empty when a text is no value of its argument.
///
QVariantList outputValues(const QList<ArgumentInfo> &outputs, const MethodResponse &response,
                          QString *problem = nullptr);

///
/// \brief Finds the stored answer of a model method.
/// \param methods Answers kept by the project.
/// \param namespaceUri URI of the model namespace.
/// \param identifier Node id within the namespace, e.g. "i=5006".
/// \return Index into methods, or -1 when the project keeps none.
///
qsizetype indexOf(const QList<ModelMethod> &methods, const QString &namespaceUri, const QString &identifier);

///
/// \brief Turns the stored answers into the answers of the served model methods.
/// \param methods Answers kept by the project.
/// \param server Server holding the models.
/// \param problems Receives why answers were skipped: an unloaded namespace, an unknown method or an output
///        text that is no value of its argument; may be null.
/// \return Answers keyed by the server node id of the method.
///
QHash<QString, MethodResponse> resolve(const QList<ModelMethod> &methods, QUaServer &server,
                                       QStringList *problems = nullptr);

}
