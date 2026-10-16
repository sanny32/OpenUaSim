// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QTest>

#include <QUaServer>

#include "testport.h"

class TestQUaServerSmoke : public QObject
{
    Q_OBJECT

private slots:
    void startsAndStops();
    void appliesSecurityModes();
};

void TestQUaServerSmoke::startsAndStops()
{
    const quint16 port = TestPort::freePort();
    QVERIFY(port != 0);

    QUaServer server;
    server.setPort(port);
    QVERIFY(server.start());
    QVERIFY(server.isRunning());

    server.stop();
    QVERIFY(!server.isRunning());
}

void TestQUaServerSmoke::appliesSecurityModes()
{
#ifdef OUASIM_HAVE_ENCRYPTION
    QUaServer server;
    const QUaMessageSecurityModes modes = QUa::MessageSecurityMode::SignAndEncrypt;
    server.setSecurityModes(modes);
    QCOMPARE(server.securityModes(), modes);
#else
    QSKIP("Built without OUASIM_WITH_ENCRYPTION");
#endif
}

QTEST_GUILESS_MAIN(TestQUaServerSmoke)

#include "test_quaserver_smoke.moc"
