// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file viewsetup.h
/// \brief Declares helpers shared by the item views of the pages.
///

#pragma once

#include <QString>

class QAbstractButton;
class QAbstractItemModel;
class QAbstractItemView;
class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QSortFilterProxyModel;
class QSplitter;
class QTableView;
class QTreeView;

namespace ViewSetup {

///
/// \brief Installs the application item delegate on a view.
/// \param view View to configure.
///
void installDelegate(QAbstractItemView *view);

///
/// \brief Installs the item delegate, left-aligns the header, sets the design row height and keeps
///        every column resizable by the user, with the last one filling the rest of the width.
/// \param view Table view to configure.
///
void configureTable(QTableView *view);

///
/// \brief Fits the columns to their contents the first time the view's model has rows, then leaves
///        the widths to the user.
/// \param view Table view whose model is already set.
///
void fitColumnsWhenFilled(QTableView *view);

///
/// \brief Widens a column whenever its contents stop fitting and never narrows it, so changing
///        values are not elided and do not make the column width jitter.
/// \param view Table view whose model is already set.
/// \param column Column to keep wide enough; growth stops at the same cap as fitColumnsWhenFilled().
///
void growColumnWithContents(QTableView *view, int column);

///
/// \brief Fills a combo box with the icons a variable can be given; item data holds the icon name.
/// \param combo Combo box to fill.
///
void fillVariableIconCombo(QComboBox *combo);

///
/// \brief Sets the editor height to show exactly the given number of text lines.
/// \param edit Editor to resize.
/// \param lines Visible line count.
///
void fitHeightToLines(QPlainTextEdit *edit, int lines);

///
/// \brief Sizes the panes of a splitter in proportion to their horizontal stretch factors.
/// \param splitter Splitter to size; panes without a stretch factor count as one.
///
void splitByStretch(QSplitter *splitter);

///
/// \brief Puts a case-insensitive, recursive text filter between a tree model and its view
///        and adds a search icon to the line edit.
/// \param view Tree view to configure.
/// \param source Model to filter.
/// \param search Line edit whose text drives the filter.
/// \return Proxy model, owned by the view.
///
QSortFilterProxyModel *attachSearch(QTreeView *view, QAbstractItemModel *source, QLineEdit *search);

///
/// \brief Puts a case-insensitive text filter over all columns between a table model and its view
///        and adds a search icon to the line edit.
/// \param view Table view to configure.
/// \param source Model to filter.
/// \param search Line edit whose text drives the filter.
/// \return Proxy model, owned by the view.
///
QSortFilterProxyModel *attachTableSearch(QTableView *view, QAbstractItemModel *source, QLineEdit *search);

///
/// \brief Expands the nodes marked expanded and selects the node marked current.
/// \param view Tree view whose model provides ItemRoles::Expanded and ItemRoles::Current.
///
/// Selects the first top-level node when no node is marked current.
///
void applyInitialState(QTreeView *view);

///
/// \brief Selects the tree node carrying an identifier.
/// \param view Tree view to update.
/// \param id Value of ItemRoles::Id to look for.
/// \return True when a node was found and selected.
///
bool selectById(QTreeView *view, const QString &id);

///
/// \brief Selects a table row and scrolls to it.
/// \param view Table view to update.
/// \param row Row to select; out-of-range rows clear the selection.
///
void selectRow(QTableView *view, int row);

///
/// \brief Gives a button the copy icon and makes it copy a line edit's text to the clipboard,
///        confirmed by a "Copied" tooltip.
/// \param button Copy button.
/// \param source Line edit whose text is copied.
///
void bindCopyButton(QAbstractButton *button, QLineEdit *source);

///
/// \brief Adds a trailing copy button inside a line edit that copies its text to the clipboard,
///        confirmed by a "Copied" tooltip.
/// \param edit Line edit to extend.
///
void addCopyAction(QLineEdit *edit);

///
/// \brief Returns the identifier of the current item of a view.
/// \param view View to inspect.
/// \return Value of ItemRoles::Id of the current index, or an empty string.
///
QString currentId(const QAbstractItemView *view);

}
