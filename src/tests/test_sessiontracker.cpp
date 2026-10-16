// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QTest>

#include "server/sessiontracker.h"

namespace {

SessionInfo session(const QString &id)
{
    SessionInfo info;
    info.sessionId = id;
    info.name = QStringLiteral("Session ") + id;
    return info;
}

const QDateTime kClosedAt(QDate(2026, 1, 1), QTime(12, 0));

}

class TestSessionTracker : public QObject
{
    Q_OBJECT

private slots:
    void opensActiveSessions();
    void closesSession();
    void closingUnknownSessionFails();
    void closeAllMovesEverySessionToHistory();
    void historyIsBounded();
    void clearForgetsEverything();
};

void TestSessionTracker::opensActiveSessions()
{
    SessionTracker tracker;
    tracker.open(session(QStringLiteral("1")));
    tracker.open(session(QStringLiteral("2")));

    QCOMPARE(tracker.activeCount(), 2);
    const QList<SessionInfo> sessions = tracker.sessions();
    QCOMPARE(sessions.first().sessionId, QStringLiteral("2"));
    QVERIFY(sessions.first().active);
}

void TestSessionTracker::closesSession()
{
    SessionTracker tracker;
    tracker.open(session(QStringLiteral("1")));
    tracker.open(session(QStringLiteral("2")));

    QVERIFY(tracker.close(QStringLiteral("2"), kClosedAt));
    QCOMPARE(tracker.activeCount(), 1);
    const QList<SessionInfo> sessions = tracker.sessions();
    QCOMPARE(sessions.size(), 2);
    QCOMPARE(sessions.first().sessionId, QStringLiteral("1"));
    QCOMPARE(sessions.last().sessionId, QStringLiteral("2"));
    QVERIFY(!sessions.last().active);
    QCOMPARE(sessions.last().lastActivity, kClosedAt);
}

void TestSessionTracker::closingUnknownSessionFails()
{
    SessionTracker tracker;
    tracker.open(session(QStringLiteral("1")));
    QVERIFY(!tracker.close(QStringLiteral("9"), kClosedAt));
    QVERIFY(tracker.close(QStringLiteral("1"), kClosedAt));
    QVERIFY(!tracker.close(QStringLiteral("1"), kClosedAt));
}

void TestSessionTracker::closeAllMovesEverySessionToHistory()
{
    SessionTracker tracker;
    tracker.open(session(QStringLiteral("1")));
    tracker.open(session(QStringLiteral("2")));
    tracker.closeAll(kClosedAt);

    QCOMPARE(tracker.activeCount(), 0);
    QCOMPARE(tracker.sessions().size(), 2);
    for (const SessionInfo &info : tracker.sessions())
        QVERIFY(!info.active);
}

void TestSessionTracker::historyIsBounded()
{
    SessionTracker tracker;
    for (int i = 0; i < SessionTracker::kClosedHistory + 5; ++i) {
        tracker.open(session(QString::number(i)));
        tracker.close(QString::number(i), kClosedAt);
    }
    const QList<SessionInfo> sessions = tracker.sessions();
    QCOMPARE(sessions.size(), SessionTracker::kClosedHistory);
    QCOMPARE(sessions.first().sessionId, QString::number(SessionTracker::kClosedHistory + 4));
}

void TestSessionTracker::clearForgetsEverything()
{
    SessionTracker tracker;
    tracker.open(session(QStringLiteral("1")));
    tracker.open(session(QStringLiteral("2")));
    tracker.close(QStringLiteral("1"), kClosedAt);
    tracker.clear();
    QVERIFY(tracker.sessions().isEmpty());
    QCOMPARE(tracker.activeCount(), 0);
}

QTEST_GUILESS_MAIN(TestSessionTracker)

#include "test_sessiontracker.moc"
