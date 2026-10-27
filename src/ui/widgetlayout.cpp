// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

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

}

///
/// \brief Saves named splitters and column headers without including child windows.
/// \param root Window whose layout is saved.
/// \param settings Settings group reserved for this window.
///
void WidgetLayout::save(const QWidget &root, QSettings &settings)
{
    for (auto *splitter : root.findChildren<QSplitter *>()) {
        if (!splitter->objectName().isEmpty() && splitter->window() == &root)
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
        if (!state.isEmpty())
            splitter->restoreState(state);
    }
    const auto headers = columnHeaders(root);
    for (auto it = headers.cbegin(); it != headers.cend(); ++it) {
        const QByteArray state = settings.value(QStringLiteral("headers/") + it.key()).toByteArray();
        ViewSetup::restoreHeaderState(it.value(), state);
    }
}
