// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file methodresponder.h
/// \brief Declares the answering of client calls to the methods of imported models.
///

#pragma once

#include <QHash>
#include <QObject>
#include <QSet>
#include <QVariantList>

#include "data/projectconfig.h"

class QUaNodeId;
class QUaServer;

///
/// \brief Answers the calls of methods of imported models, which have no implementation, with the configured
///        response or, without one, with Good and the default output values.
///
class MethodResponder : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Creates a responder that answers no method yet.
    /// \param server Server holding the methods; must outlive the responder or the next setServer().
    /// \param parent Owning QObject.
    ///
    explicit MethodResponder(QUaServer *server, QObject *parent = nullptr);

    ///
    /// \brief Forgets the answered methods and answers the methods bound later on another server.
    /// \param server Server holding the methods; must outlive the responder or the next call.
    ///
    void setServer(QUaServer *server);

    ///
    /// \brief Answers the calls of the methods among nodes that a model added.
    /// \param nodeIds Added nodes; nodes that are no methods without implementation are skipped.
    ///
    void bind(const QList<QUaNodeId> &nodeIds);

    ///
    /// \brief Tells whether the calls of a method are answered.
    /// \param methodId Server node id of the method.
    /// \return True for a method bind() took.
    ///
    bool isBound(const QString &methodId) const;

    ///
    /// \brief Replaces every configured response.
    /// \param responses Responses keyed by the server node id of the method; other methods answer Good with the
    ///        default output values.
    ///
    void setResponses(const QHash<QString, MethodResponse> &responses);

    ///
    /// \brief Replaces the response of one method.
    /// \param methodId Server node id of the method.
    /// \param response Response to later calls.
    ///
    void setResponse(const QString &methodId, const MethodResponse &response);

signals:
    ///
    /// \brief Announces an answered call.
    /// \param methodId Server node id of the called method.
    /// \param result Inputs, status and outputs of the call as text.
    ///
    void methodCalled(const QString &methodId, const QString &result);

private:
    quint32 answer(const QString &methodId, const QUaNodeId &objectId, const QVariantList &inputs,
                   QVariantList &outputs);

    QUaServer *_server;
    QSet<QString> _bound;
    QHash<QString, MethodResponse> _responses;
};
