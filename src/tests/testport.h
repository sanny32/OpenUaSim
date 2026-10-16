// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file testport.h
/// \brief Declares the helper that picks a TCP port for servers started by tests.
///

#pragma once

#include <QHostAddress>
#include <QTcpServer>

namespace TestPort {

///
/// \brief Returns a TCP port that is free on the loopback interface right now.
/// \return Port number; 0 if the system refused to allocate one.
///
inline quint16 freePort()
{
    QTcpServer probe;
    if (!probe.listen(QHostAddress::Any, 0))
        return 0;
    return probe.serverPort();
}

}
