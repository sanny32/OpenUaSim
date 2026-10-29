// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file methodresponder.cpp
/// \brief Implements the answering of client calls to the methods of imported models.
///

#include <QUaServer>

#include "formatting.h"
#include "methodresponder.h"
#include "methodscenario.h"
#include "modelmethods.h"
#include "project/methodstatuses.h"

///
/// \brief Creates a responder that answers no method yet.
/// \param server Server holding the methods; must outlive the responder or the next setServer().
/// \param addressSpace Managed variables used for assignments; must outlive the responder.
/// \param parent Owning QObject.
///
MethodResponder::MethodResponder(QUaServer *server, AddressSpace *addressSpace, QObject *parent)
    : QObject(parent)
    , _server(server)
    , _addressSpace(addressSpace)
{
}

///
/// \brief Forgets the answered methods and answers the methods bound later on another server.
/// \param server Server holding the methods; must outlive the responder or the next call.
///
void MethodResponder::setServer(QUaServer *server)
{
    _server = server;
    _bound.clear();
}

///
/// \brief Answers the calls of the methods among nodes that a model added.
/// \param nodeIds Added nodes; nodes that are no methods without implementation are skipped.
///
void MethodResponder::bind(const QList<QUaNodeId> &nodeIds)
{
    for (const QUaNodeId &nodeId : nodeIds) {
        const QString methodId = nodeId.toXmlString();
        // setMethodHandler() refuses every node that is no method without implementation.
        const bool bound = _server->setMethodHandler(
            nodeId, [this, methodId](const QUaNodeId &objectId, const QVariantList &inputs, QVariantList &outputs) {
                return answer(methodId, objectId, inputs, outputs);
            });
        if (bound)
            _bound.insert(methodId);
    }
}

///
/// \brief Tells whether the calls of a method are answered.
/// \param methodId Server node id of the method.
/// \return True for a method bind() took.
///
bool MethodResponder::isBound(const QString &methodId) const
{
    return _bound.contains(methodId);
}

///
/// \brief Replaces every configured response.
/// \param responses Responses keyed by the server node id of the method; other methods answer Good with the
///        default output values.
///
void MethodResponder::setResponses(const QHash<QString, MethodResponse> &responses)
{
    _responses = responses;
}

///
/// \brief Replaces the response of one method.
/// \param methodId Server node id of the method.
/// \param response Response to later calls.
///
void MethodResponder::setResponse(const QString &methodId, const MethodResponse &response)
{
    _responses.insert(methodId, response);
}

///
/// \brief Answers a call with the configured response and announces it.
/// \param methodId Server node id of the called method.
/// \param objectId Object the method is called on.
/// \param inputs Input arguments.
/// \param outputs Receives the output values; invalid ones get the default value of their data type.
/// \return Status code of the response.
///
quint32 MethodResponder::answer(const QString &methodId, const QUaNodeId &objectId, const QVariantList &inputs,
                                QVariantList &outputs)
{
    MethodInfo method = ModelMethods::describe(*_server, methodId);
    method.response = _responses.value(methodId);
    QHash<QString, QVariant> changes;
    QString error;
    const quint32 status = MethodScenario::execute(*_server, _addressSpace, method, inputs, outputs, changes, error);
    if (!changes.isEmpty())
        emit valuesSetByMethod(changes);
    emit methodCalled(methodId, tr("inputs %1 on %2, answered %3")
                                    .arg(Formatting::value(inputs), objectId.toXmlString(),
                                         error.isEmpty() ? MethodStatuses::name(status) : error));
    return status;
}
