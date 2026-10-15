// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QSignalSpy>
#include <QStringListModel>
#include <QTest>

#include "logfilterproxymodel.h"
#include "logmodel.h"

namespace {

LogEntry entry(LogEntry::Level level, LogEntry::Category category, const QString &message)
{
    LogEntry result;
    result.level = level;
    result.category = category;
    result.message = message;
    return result;
}

QStringList messages(const QAbstractItemModel &model)
{
    QStringList result;
    for (int row = 0; row < model.rowCount(); ++row)
        result.append(model.index(row, LogModel::ColMessage).data().toString());
    return result;
}

}

class TestLogFilterProxyModel : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void showsEverythingByDefault();
    void hidesLevels();
    void hidesCategories();
    void combinesLevelAndCategoryFilters();
    void ignoresRedundantChanges();
    void resolvesEntries();
    void acceptsForeignSourceModels();

private:
    LogModel _model;
    LogFilterProxyModel _filter;
};

void TestLogFilterProxyModel::init()
{
    _model.setItems({
        entry(LogEntry::Level::Info, LogEntry::Category::Server, QStringLiteral("started")),
        entry(LogEntry::Level::Warning, LogEntry::Category::Session, QStringLiteral("slow")),
        entry(LogEntry::Level::Error, LogEntry::Category::Security, QStringLiteral("denied")),
        entry(LogEntry::Level::Info, LogEntry::Category::DataChanges, QStringLiteral("value")),
    });
    _filter.setSourceModel(&_model);
    for (auto level : { LogEntry::Level::Info, LogEntry::Level::Warning, LogEntry::Level::Error })
        _filter.setLevelVisible(level, true);
    for (auto category : { LogEntry::Category::Server, LogEntry::Category::Session,
                           LogEntry::Category::Security, LogEntry::Category::DataChanges })
        _filter.setCategoryVisible(category, true);
}

void TestLogFilterProxyModel::showsEverythingByDefault()
{
    LogFilterProxyModel fresh;
    fresh.setSourceModel(&_model);
    QCOMPARE(fresh.rowCount(), 4);
    QVERIFY(fresh.isLevelVisible(LogEntry::Level::Error));
    QVERIFY(fresh.isCategoryVisible(LogEntry::Category::Security));
}

void TestLogFilterProxyModel::hidesLevels()
{
    _filter.setLevelVisible(LogEntry::Level::Info, false);
    QVERIFY(!_filter.isLevelVisible(LogEntry::Level::Info));
    QCOMPARE(messages(_filter), QStringList({ QStringLiteral("slow"), QStringLiteral("denied") }));

    _filter.setLevelVisible(LogEntry::Level::Info, true);
    QCOMPARE(_filter.rowCount(), 4);
}

void TestLogFilterProxyModel::hidesCategories()
{
    _filter.setCategoryVisible(LogEntry::Category::Security, false);
    _filter.setCategoryVisible(LogEntry::Category::DataChanges, false);
    QCOMPARE(messages(_filter), QStringList({ QStringLiteral("started"), QStringLiteral("slow") }));
}

void TestLogFilterProxyModel::combinesLevelAndCategoryFilters()
{
    _filter.setLevelVisible(LogEntry::Level::Warning, false);
    _filter.setCategoryVisible(LogEntry::Category::Server, false);
    QCOMPARE(messages(_filter), QStringList({ QStringLiteral("denied"), QStringLiteral("value") }));
}

void TestLogFilterProxyModel::ignoresRedundantChanges()
{
    QSignalSpy layout(&_filter, &LogFilterProxyModel::rowsRemoved);
    _filter.setLevelVisible(LogEntry::Level::Info, true);
    _filter.setCategoryVisible(LogEntry::Category::Server, true);
    QCOMPARE(layout.count(), 0);
    QCOMPARE(_filter.rowCount(), 4);
}

void TestLogFilterProxyModel::resolvesEntries()
{
    _filter.setLevelVisible(LogEntry::Level::Info, false);
    const LogEntry *first = _filter.entry(_filter.index(0, LogModel::ColTime));
    QVERIFY(first);
    QCOMPARE(first->message, QStringLiteral("slow"));
    QVERIFY(!_filter.entry(QModelIndex()));
}

void TestLogFilterProxyModel::acceptsForeignSourceModels()
{
    QStringListModel strings({ QStringLiteral("a"), QStringLiteral("b") });
    LogFilterProxyModel filter;
    filter.setSourceModel(&strings);
    filter.setLevelVisible(LogEntry::Level::Info, false);
    QCOMPARE(filter.rowCount(), 2);
    QVERIFY(!filter.entry(filter.index(0, 0)));
}

QTEST_GUILESS_MAIN(TestLogFilterProxyModel)
#include "test_logfilterproxymodel.moc"
