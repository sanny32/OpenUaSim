// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <numeric>

#include <QDataStream>
#include <QHeaderView>
#include <QSettings>
#include <QSplitter>
#include <QTableView>
#include <QTreeView>

#include "viewsetup.h"
#include "widgetlayout.h"

namespace {

///
/// \brief Builds a stable settings path that distinguishes identically named controls on different pages.
///
QString widgetPath(const QWidget &widget, const QWidget &root)
{
    QStringList names;
    for (const QWidget *part = &widget; part && part != &root; part = part->parentWidget()) {
        if (!part->objectName().isEmpty())
            names.prepend(part->objectName());
    }
    return names.join(QLatin1Char('/'));
}

///
/// \brief Selects column headers of named tables and trees belonging to this window.
///
QMap<QString, QHeaderView *> columnHeaders(const QWidget &root)
{
    QMap<QString, QHeaderView *> headers;
    for (auto *view : root.findChildren<QAbstractItemView *>()) {
        if (view->objectName().isEmpty() || view->window() != &root)
            continue;
        QHeaderView *header = nullptr;
        if (auto *table = qobject_cast<QTableView *>(view))
            header = table->horizontalHeader();
        else if (auto *tree = qobject_cast<QTreeView *>(view))
            header = tree->header();
        if (header)
            headers.insert(widgetPath(*view, root), header);
    }
    return headers;
}

///
/// \brief Reads the pane sizes from a state written by QSplitter::saveState().
/// \return Sizes, or an empty list when the state is unreadable.
///
QList<int> savedSizes(const QByteArray &state)
{
    QDataStream stream(state);
    stream.setVersion(QDataStream::Qt_5_0);
    qint32 marker = 0;
    qint32 version = 0;
    QList<int> sizes;
    stream >> marker >> version >> sizes;
    return stream.status() == QDataStream::Ok ? sizes : QList<int>();
}

///
/// \brief Gives the panes of a laid-out splitter the saved sizes scaled to its current extent.
/// \param splitter Splitter to size.
/// \param saved Saved pane sizes; ignored when they do not match the panes or already fill the splitter.
///
void fitToExtent(QSplitter *splitter, const QList<int> &saved)
{
    const QList<int> current = splitter->sizes();
    const qint64 savedTotal = std::accumulate(saved.cbegin(), saved.cend(), qint64(0));
    const int total = std::accumulate(current.cbegin(), current.cend(), 0);
    if (saved.size() != current.size() || savedTotal <= 0 || savedTotal == total)
        return;
    QList<int> scaled;
    int used = 0;
    for (qsizetype i = 0; i + 1 < saved.size(); ++i) {
        scaled.append(qRound(double(saved.at(i)) * total / savedTotal));
        used += scaled.last();
    }
    scaled.append(total - used);
    splitter->setSizes(scaled);
}

}

///
/// \brief Saves named splitters and column headers without including child windows.
/// \param root Window whose layout is saved.
/// \param settings Settings group reserved for this window.
///
void WidgetLayout::save(const QWidget &root, QSettings &settings)
{
    for (auto *splitter : root.findChildren<QSplitter *>()) {
        // A splitter on a page never shown still holds its unscaled initial sizes; keep the saved state instead.
        if (!splitter->objectName().isEmpty() && splitter->window() == &root
            && !splitter->testAttribute(Qt::WA_PendingResizeEvent))
            settings.setValue(QStringLiteral("splitters/") + widgetPath(*splitter, root), splitter->saveState());
    }
    const auto headers = columnHeaders(root);
    for (auto it = headers.cbegin(); it != headers.cend(); ++it)
        settings.setValue(QStringLiteral("headers/") + it.key(), it.value()->saveState());
}

///
/// \brief Restores named splitters and column headers, retaining defaults for missing or invalid state.
/// \param root Window whose layout is restored.
/// \param settings Settings group reserved for this window.
///
void WidgetLayout::restore(QWidget &root, QSettings &settings)
{
    for (auto *splitter : root.findChildren<QSplitter *>()) {
        if (splitter->objectName().isEmpty() || splitter->window() != &root)
            continue;
        const QByteArray state = settings.value(QStringLiteral("splitters/") + widgetPath(*splitter, root)).toByteArray();
        if (state.isEmpty() || !splitter->restoreState(state))
            continue;
        // A laid-out splitter with pinned side panes squeezes sizes that do not fill it into its first pane.
        if (!splitter->testAttribute(Qt::WA_PendingResizeEvent))
            fitToExtent(splitter, savedSizes(state));
    }
    const auto headers = columnHeaders(root);
    for (auto it = headers.cbegin(); it != headers.cend(); ++it) {
        const QByteArray state = settings.value(QStringLiteral("headers/") + it.key()).toByteArray();
        ViewSetup::restoreHeaderState(it.value(), state);
    }
}
