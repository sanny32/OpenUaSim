// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file viewsetup.cpp
/// \brief Implements helpers shared by the item views of the pages.
///

#include <QAbstractButton>
#include <QAction>
#include <QCoreApplication>
#include <QClipboard>
#include <QGuiApplication>
#include <QHeaderView>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QTableView>
#include <QTreeView>

#include "appicons.h"
#include "itemdelegate.h"
#include "itemroles.h"
#include "viewsetup.h"

namespace ViewSetup {

namespace {

constexpr int kTableRowHeight = 34;
constexpr int kSplitScale = 100;

///
/// \brief Puts a case-insensitive text filter between a model and a view, driven by a line edit.
/// \param view View to show the filtered model; it owns the proxy.
/// \param source Model to filter.
/// \param search Line edit whose text drives the filter; it gets a search icon.
/// \return Proxy model.
///
QSortFilterProxyModel *createSearchProxy(QAbstractItemView *view, QAbstractItemModel *source,
                                         QLineEdit *search)
{
    auto *proxy = new QSortFilterProxyModel(view);
    proxy->setSourceModel(source);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    view->setModel(proxy);
    search->addAction(AppIcons::icon(QStringLiteral("search")), QLineEdit::LeadingPosition);
    QObject::connect(search, &QLineEdit::textChanged, proxy, &QSortFilterProxyModel::setFilterFixedString);
    return proxy;
}

///
/// \brief Expands marked nodes below a parent, depth-first.
/// \param view Tree view to update.
/// \param parent Parent whose children are visited.
/// \param current Receives the first node marked current.
///
void expandMarked(QTreeView *view, const QModelIndex &parent, QModelIndex &current)
{
    const QAbstractItemModel *model = view->model();
    for (int row = 0; row < model->rowCount(parent); ++row) {
        const QModelIndex index = model->index(row, 0, parent);
        if (index.data(ItemRoles::Expanded).toBool())
            view->expand(index);
        if (!current.isValid() && index.data(ItemRoles::Current).toBool())
            current = index;
        expandMarked(view, index, current);
    }
}

}

///
/// \brief Installs the application item delegate on a view.
/// \param view View to configure.
///
void installDelegate(QAbstractItemView *view)
{
    view->setItemDelegate(new ItemDelegate(view));
}

///
/// \brief Installs the item delegate, left-aligns the header and sets the design row height.
/// \param view Table view to configure.
/// \param sizing How the columns share the view width.
///
void configureTable(QTableView *view, ColumnSizing sizing)
{
    installDelegate(view);
    view->verticalHeader()->setDefaultSectionSize(kTableRowHeight);
    QHeaderView *header = view->horizontalHeader();
    header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (sizing == ColumnSizing::Stretch) {
        header->setSectionResizeMode(QHeaderView::Stretch);
        // A stretched last section on top of evenly stretched ones overflows into a scroll bar.
        header->setStretchLastSection(false);
    }
}

///
/// \brief Sizes the panes of a splitter in proportion to their horizontal stretch factors.
/// \param splitter Splitter to size; panes without a stretch factor count as one.
///
void splitByStretch(QSplitter *splitter)
{
    // QSplitter scales oversized sizes down to the available width, keeping their ratio.
    QList<int> sizes;
    for (int i = 0; i < splitter->count(); ++i)
        sizes.append(qMax(1, splitter->widget(i)->sizePolicy().horizontalStretch()) * kSplitScale);
    splitter->setSizes(sizes);
}

///
/// \brief Puts a case-insensitive, recursive text filter between a tree model and its view
///        and adds a search icon to the line edit.
/// \param view Tree view to configure.
/// \param source Model to filter.
/// \param search Line edit whose text drives the filter.
/// \return Proxy model, owned by the view.
///
QSortFilterProxyModel *attachSearch(QTreeView *view, QAbstractItemModel *source, QLineEdit *search)
{
    QSortFilterProxyModel *proxy = createSearchProxy(view, source, search);
    proxy->setRecursiveFilteringEnabled(true);
    proxy->setAutoAcceptChildRows(true);
    QObject::connect(search, &QLineEdit::textChanged, view, &QTreeView::expandAll);
    return proxy;
}

///
/// \brief Puts a case-insensitive text filter over all columns between a table model and its view
///        and adds a search icon to the line edit.
/// \param view Table view to configure.
/// \param source Model to filter.
/// \param search Line edit whose text drives the filter.
/// \return Proxy model, owned by the view.
///
QSortFilterProxyModel *attachTableSearch(QTableView *view, QAbstractItemModel *source, QLineEdit *search)
{
    QSortFilterProxyModel *proxy = createSearchProxy(view, source, search);
    proxy->setFilterKeyColumn(-1);
    return proxy;
}

///
/// \brief Expands the nodes marked expanded and selects the node marked current.
/// \param view Tree view whose model provides ItemRoles::Expanded and ItemRoles::Current.
///
/// Selects the first top-level node when no node is marked current.
///
void applyInitialState(QTreeView *view)
{
    QModelIndex current;
    expandMarked(view, QModelIndex(), current);
    if (!current.isValid())
        current = view->model()->index(0, 0);
    if (current.isValid())
        view->setCurrentIndex(current);
}

///
/// \brief Selects the tree node carrying an identifier.
/// \param view Tree view to update.
/// \param id Value of ItemRoles::Id to look for.
/// \return True when a node was found and selected.
///
bool selectById(QTreeView *view, const QString &id)
{
    const QAbstractItemModel *model = view->model();
    const QModelIndexList found = model->match(model->index(0, 0), ItemRoles::Id, id, 1,
                                               Qt::MatchExactly | Qt::MatchRecursive);
    if (found.isEmpty())
        return false;
    view->setCurrentIndex(found.first());
    view->scrollTo(found.first());
    return true;
}

///
/// \brief Selects a table row and scrolls to it.
/// \param view Table view to update.
/// \param row Row to select; out-of-range rows clear the selection.
///
void selectRow(QTableView *view, int row)
{
    const QModelIndex index = view->model()->index(row, 0);
    if (!index.isValid()) {
        view->clearSelection();
        view->setCurrentIndex(QModelIndex());
        return;
    }
    view->setCurrentIndex(index);
    view->scrollTo(index);
}

///
/// \brief Gives a button the copy icon and makes it copy a line edit's text to the clipboard.
/// \param button Copy button.
/// \param source Line edit whose text is copied.
///
void bindCopyButton(QAbstractButton *button, QLineEdit *source)
{
    button->setIcon(AppIcons::icon(QStringLiteral("copy")));
    QObject::connect(button, &QAbstractButton::clicked, source, [source] {
        QGuiApplication::clipboard()->setText(source->text());
    });
}

///
/// \brief Adds a trailing copy button inside a line edit that copies its text to the clipboard.
/// \param edit Line edit to extend.
///
void addCopyAction(QLineEdit *edit)
{
    QAction *copy = edit->addAction(AppIcons::icon(QStringLiteral("copy")), QLineEdit::TrailingPosition);
    copy->setToolTip(QCoreApplication::translate("ViewSetup", "Copy"));
    QObject::connect(copy, &QAction::triggered, edit, [edit] {
        QGuiApplication::clipboard()->setText(edit->text());
    });
}

///
/// \brief Returns the identifier of the current item of a view.
/// \param view View to inspect.
/// \return Value of ItemRoles::Id of the current index, or an empty string.
///
QString currentId(const QAbstractItemView *view)
{
    return view->currentIndex().siblingAtColumn(0).data(ItemRoles::Id).toString();
}

}
