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
class QHeaderView;
class QByteArray;
class QLineEdit;
class QLabel;
class QPlainTextEdit;
class QSortFilterProxyModel;
class QSplitter;
class QTableView;
class QTreeView;
class QWidget;

namespace ViewSetup {

///
/// \brief Restores user column sizes and prevents automatic fitting from replacing them.
/// \param header Column header to restore.
/// \param state Previously saved header state.
/// \return True when the state was restored; false for a state saved with a different column count.
///
bool restoreHeaderState(QHeaderView *header, const QByteArray &state);

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
///        the widths to the user; the table scrolls horizontally when the last column does not fit.
/// \param view Table view whose model is already set.
///
void fitColumnsWhenFilled(QTableView *view);

///
/// \brief Keeps a column at least as wide as a sample value when fitColumnsWhenFilled() sizes it, so a
///        column that is often empty still has room for its typical value.
/// \param view Table view whose model is already set.
/// \param column Column to reserve the width for.
/// \param sample Typical value of the column.
///
void reserveColumnWidth(QTableView *view, int column, const QString &sample);

///
/// \brief Widens a column whenever its contents stop fitting and never narrows it, so changing
///        values are not elided and do not make the column width jitter.
/// \param view Table view whose model is already set.
/// \param column Column to keep wide enough; growth stops at the same cap as fitColumnsWhenFilled().
///
void growColumnWithContents(QTableView *view, int column);

///
/// \brief Associates translated access choices with the stable values stored in projects.
/// \param combo Access selector whose items are Read, Write and Read/Write, in that order.
///
void configureAccessCombo(QComboBox *combo);

///
/// \brief Fills a combo box with the icons a variable can be given; item data holds the icon name.
/// \param combo Combo box to fill.
///
void fillVariableIconCombo(QComboBox *combo);

///
/// \brief Relabels a combo box filled by fillVariableIconCombo() in the current language.
/// \param combo Combo box to relabel; the selection is kept.
///
void retranslateVariableIconCombo(QComboBox *combo);

///
/// \brief Sets the editor height to show exactly the given number of text lines.
/// \param edit Editor to resize.
/// \param lines Visible line count.
///
void fitHeightToLines(QPlainTextEdit *edit, int lines);

///
/// \brief Sizes the panes of a splitter in proportion to their horizontal stretch factors and keeps
///        the side panes at that width when the splitter is resized later: the last pane, and the
///        first one as well when panes lie between them.
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
/// \brief Copies a label's complete text and confirms it with a "Copied" tooltip.
/// \param button Copy button.
/// \param source Label whose text is copied.
///
void bindCopyButton(QAbstractButton *button, QLabel *source);

///
/// \brief Enables a button whenever an editor inside a container changes; the caller disables it again
///        after loading or storing the values.
/// \param editors Container whose line edits, combo boxes, spin boxes and check boxes count as editors.
/// \param button Button to enable, typically Apply.
///
void enableOnEdit(QWidget *editors, QAbstractButton *button);

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

///
/// \brief Configures a tree with table columns and the application delegate.
/// \param view Tree view to configure.
///
void configureTable(QTreeView *view);

///
/// \brief Fits tree columns once rows become available.
/// \param view Tree view whose model is already set.
///
void fitColumnsWhenFilled(QTreeView *view);

///
/// \brief Keeps a tree column wide enough for changing values.
/// \param view Tree view whose model is already set.
/// \param column Column to grow.
///
void growColumnWithContents(QTreeView *view, int column);

///
/// \brief Selects a root row in a tree with table columns.
/// \param view Tree view to update.
/// \param row Root row to select.
///
void selectRow(QTreeView *view, int row);

}
