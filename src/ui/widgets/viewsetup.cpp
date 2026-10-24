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
#include <QComboBox>
#include <QGuiApplication>
#include <QHeaderView>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QResizeEvent>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QTableView>
#include <QToolButton>
#include <QToolTip>
#include <QTreeView>

#include "appicons.h"
#include "itemdelegate.h"
#include "itemroles.h"
#include "viewsetup.h"

namespace ViewSetup {

namespace {

constexpr int kTableRowHeight = 34;
constexpr int kMaxFittedColumnWidth = 320;
constexpr int kSplitScale = 100;

///
/// \brief Keeps the side panes of a splitter at their widths once the splitter has been laid out,
///        so resizing the splitter only resizes the panes between them.
///
class SidePanePin : public QObject
{
public:
    explicit SidePanePin(QSplitter *splitter);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QSplitter *_splitter;
};

///
/// \brief Watches a splitter's resizes; the pin is owned by the splitter.
/// \param splitter Splitter whose side panes to pin.
///
SidePanePin::SidePanePin(QSplitter *splitter)
    : QObject(splitter)
    , _splitter(splitter)
{
    splitter->installEventFilter(this);
}

///
/// \brief Drops the stretch factors of the last pane and, when panes lie between, of the first one
///        before the first resize that follows the initial layout.
/// \param watched Watched splitter.
/// \param event Event delivered to it.
/// \return Always false, so the splitter still handles the event.
///
bool SidePanePin::eventFilter(QObject *watched, QEvent *event)
{
    // The initial layout distributes the set sizes by stretch; a zero stretch there would keep the raw value.
    if (event->type() == QEvent::Resize && static_cast<QResizeEvent *>(event)->oldSize().isValid()) {
        // An unstretched pane keeps its last requested size, which can differ from the laid-out one.
        _splitter->setSizes(_splitter->sizes());
        _splitter->setStretchFactor(_splitter->count() - 1, 0);
        if (_splitter->count() > 2)
            _splitter->setStretchFactor(0, 0);
        _splitter->removeEventFilter(this);
        deleteLater();
    }
    return QObject::eventFilter(watched, event);
}

///
/// \brief Copies text to the clipboard and confirms it with a "Copied" tooltip under the clicked button.
/// \param text Text to copy.
/// \param button Button the tooltip appears under.
///
void copyToClipboard(const QString &text, QWidget *button)
{
    QGuiApplication::clipboard()->setText(text);
    QToolTip::showText(button->mapToGlobal(QPoint(0, button->height())),
                       QCoreApplication::translate("ViewSetup", "Copied"), button);
}

///
/// \brief Finds the tool button a line edit shows for one of its actions.
/// \param action Action added to a line edit.
/// \return The button, or nullptr when the action has none.
///
QToolButton *actionButton(const QAction *action)
{
    for (QObject *object : action->associatedObjects()) {
        if (auto *button = qobject_cast<QToolButton *>(object))
            return button;
    }
    return nullptr;
}

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
/// \brief Returns the column header of a table or tree.
/// \param view Table or tree view.
/// \return The horizontal header.
///
QHeaderView *columnHeader(QAbstractItemView *view)
{
    if (auto *table = qobject_cast<QTableView *>(view))
        return table->horizontalHeader();
    return qobject_cast<QTreeView *>(view)->header();
}

///
/// \brief Fits a column using the concrete view's sizing rules.
/// \param view Table or tree view.
/// \param column Column to resize.
///
void resizeColumn(QAbstractItemView *view, int column)
{
    if (auto *table = qobject_cast<QTableView *>(view))
        table->resizeColumnToContents(column);
    else
        qobject_cast<QTreeView *>(view)->resizeColumnToContents(column);
}

///
/// \brief Sizes the columns to their contents, capped but for the last one so one long value cannot
///        push the others out of view.
/// \param view Table view to size.
///
void fitColumnsToContents(QAbstractItemView *view)
{
    QHeaderView *header = columnHeader(view);
    const int last = header->count() - 1;
    for (int column = 0; column < last; ++column) {
        if (header->isSectionHidden(column))
            continue;
        resizeColumn(view, column);
        if (header->sectionSize(column) > kMaxFittedColumnWidth)
            header->resizeSection(column, kMaxFittedColumnWidth);
    }
    // The stretched last section never shrinks below this width, so contents that do not fit scroll.
    resizeColumn(view, last);
}

///
/// \brief Fits the columns of a table once, as soon as its model has rows.
///
class ColumnFitter : public QObject
{
public:
    explicit ColumnFitter(QAbstractItemView *view);
    void fitOnceFilled();

private:
    QAbstractItemView *_view;
};

///
/// \brief Watches the model of a table for its first rows.
/// \param view Table view to size; it owns the fitter.
///
ColumnFitter::ColumnFitter(QAbstractItemView *view)
    : QObject(view)
    , _view(view)
{
    connect(view->model(), &QAbstractItemModel::modelReset, this, &ColumnFitter::fitOnceFilled);
    connect(view->model(), &QAbstractItemModel::rowsInserted, this, &ColumnFitter::fitOnceFilled);
}

///
/// \brief Fits the columns and retires once the model has rows, so later data keeps the user's widths.
///
void ColumnFitter::fitOnceFilled()
{
    if (_view->model()->rowCount() == 0)
        return;
    fitColumnsToContents(_view);
    disconnect(_view->model(), nullptr, this, nullptr);
    deleteLater();
}

///
/// \brief Widens one column of a table whenever its contents stop fitting.
///
class ColumnGrower : public QObject
{
public:
    ColumnGrower(QAbstractItemView *view, int column);
    void growOnChange(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void grow();

private:
    QAbstractItemView *_view;
    int _column;
};

///
/// \brief Watches the model of a table for new or changed rows.
/// \param view Table view to size; it owns the grower.
/// \param column Column to keep wide enough.
///
ColumnGrower::ColumnGrower(QAbstractItemView *view, int column)
    : QObject(view)
    , _view(view)
    , _column(column)
{
    connect(view->model(), &QAbstractItemModel::dataChanged, this, &ColumnGrower::growOnChange);
    connect(view->model(), &QAbstractItemModel::modelReset, this, &ColumnGrower::grow);
    connect(view->model(), &QAbstractItemModel::rowsInserted, this, &ColumnGrower::grow);
}

///
/// \brief Grows the column when a change touches it.
/// \param topLeft First changed cell.
/// \param bottomRight Last changed cell.
///
void ColumnGrower::growOnChange(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (topLeft.column() <= _column && _column <= bottomRight.column())
        grow();
}

///
/// \brief Widens the column to its contents, capped, when they no longer fit.
///
void ColumnGrower::grow()
{
    QHeaderView *header = columnHeader(_view);
    // QTableView narrows the public QAbstractItemView::sizeHintForColumn() override to protected.
    const QAbstractItemView *view = _view;
    const int needed = qMin(qMax(view->sizeHintForColumn(_column), header->sectionSizeHint(_column)),
                            kMaxFittedColumnWidth);
    if (needed > header->sectionSize(_column))
        header->resizeSection(_column, needed);
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
/// \brief Installs the item delegate, left-aligns the header, sets the design row height and keeps
///        every column resizable by the user, with the last one filling the rest of the width.
/// \param view Table view to configure.
///
void configureTable(QTableView *view)
{
    installDelegate(view);
    view->verticalHeader()->setDefaultSectionSize(kTableRowHeight);
    QHeaderView *header = columnHeader(view);
    header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setStretchLastSection(true);
}

///
/// \brief Fits the columns to their contents the first time the view's model has rows, then leaves
///        the widths to the user.
/// \param view Table view whose model is already set.
///
void fitColumnsWhenFilled(QTableView *view)
{
    auto *fitter = new ColumnFitter(view);
    fitter->fitOnceFilled();
}

///
/// \brief Widens a column whenever its contents stop fitting and never narrows it, so changing
///        values are not elided and do not make the column width jitter.
/// \param view Table view whose model is already set.
/// \param column Column to keep wide enough; growth stops at the same cap as fitColumnsWhenFilled().
///
void growColumnWithContents(QTableView *view, int column)
{
    auto *grower = new ColumnGrower(view, column);
    grower->grow();
}

///
/// \brief Fills a combo box with the icons a variable can be given; item data holds the icon name.
/// \param combo Combo box to fill.
///
void fillVariableIconCombo(QComboBox *combo)
{
    for (const AppIcons::NamedIcon &icon : AppIcons::variableIcons())
        combo->addItem(AppIcons::icon(icon.name), icon.label, icon.name);
}

///
/// \brief Relabels a combo box filled by fillVariableIconCombo() in the current language.
/// \param combo Combo box to relabel; the selection is kept.
///
void retranslateVariableIconCombo(QComboBox *combo)
{
    for (const AppIcons::NamedIcon &icon : AppIcons::variableIcons()) {
        const int index = combo->findData(icon.name);
        if (index >= 0)
            combo->setItemText(index, icon.label);
    }
}

///
/// \brief Sets the editor height to show exactly the given number of text lines.
/// \param edit Editor to resize.
/// \param lines Visible line count.
///
void fitHeightToLines(QPlainTextEdit *edit, int lines)
{
    const int documentMargin = qRound(edit->document()->documentMargin());
    // QPlainTextEdit scrolls unless the viewport fits the first block's margins, the bottom margin and 1px.
    edit->setFixedHeight(lines * edit->fontMetrics().lineSpacing() + 3 * documentMargin + 1
                         + 2 * edit->frameWidth());
}

///
/// \brief Sizes the panes of a splitter in proportion to their horizontal stretch factors and keeps
///        the side panes at that width when the splitter is resized later: the last pane, and the
///        first one as well when panes lie between them.
/// \param splitter Splitter to size; panes without a stretch factor count as one.
///
void splitByStretch(QSplitter *splitter)
{
    // QSplitter scales oversized sizes down to the available width, keeping their ratio.
    QList<int> sizes;
    for (int i = 0; i < splitter->count(); ++i)
        sizes.append(qMax(1, splitter->widget(i)->sizePolicy().horizontalStretch()) * kSplitScale);
    splitter->setSizes(sizes);
    new SidePanePin(splitter);
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
/// \brief Gives a button the copy icon and makes it copy a line edit's text to the clipboard,
///        confirmed by a "Copied" tooltip.
/// \param button Copy button.
/// \param source Line edit whose text is copied.
///
void bindCopyButton(QAbstractButton *button, QLineEdit *source)
{
    button->setIcon(AppIcons::icon(QStringLiteral("copy")));
    button->setCursor(Qt::PointingHandCursor);
    QObject::connect(button, &QAbstractButton::clicked, source,
                     [button, source] { copyToClipboard(source->text(), button); });
}

///
/// \brief Adds a trailing copy button inside a line edit that copies its text to the clipboard,
///        confirmed by a "Copied" tooltip.
/// \param edit Line edit to extend.
///
void addCopyAction(QLineEdit *edit)
{
    QAction *copy = edit->addAction(AppIcons::icon(QStringLiteral("copy")), QLineEdit::TrailingPosition);
    copy->setToolTip(QCoreApplication::translate("ViewSetup", "Copy"));
    QToolButton *button = actionButton(copy);
    QWidget *anchor = button ? static_cast<QWidget *>(button) : edit;
    if (button)
        button->setCursor(Qt::PointingHandCursor);
    QObject::connect(copy, &QAction::triggered, edit, [edit, anchor] { copyToClipboard(edit->text(), anchor); });
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

///
/// \brief Configures a tree with table columns and the application delegate.
/// \param view Tree view to configure.
///
void configureTable(QTreeView *view)
{
    installDelegate(view);
    view->setUniformRowHeights(true);
    view->header()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    view->header()->setSectionResizeMode(QHeaderView::Interactive);
    view->header()->setStretchLastSection(true);
}

///
/// \brief Fits tree columns once rows become available.
/// \param view Tree view whose model is already set.
///
void fitColumnsWhenFilled(QTreeView *view)
{
    auto *fitter = new ColumnFitter(view);
    fitter->fitOnceFilled();
}

///
/// \brief Keeps a tree column wide enough for changing values.
/// \param view Tree view whose model is already set.
/// \param column Column to grow.
///
void growColumnWithContents(QTreeView *view, int column)
{
    auto *grower = new ColumnGrower(view, column);
    grower->grow();
}

///
/// \brief Selects a root row in a tree with table columns.
/// \param view Tree view to update.
/// \param row Root row to select.
///
void selectRow(QTreeView *view, int row)
{
    const QModelIndex index = view->model()->index(row, 0);
    if (!index.isValid())
        view->clearSelection();
    view->setCurrentIndex(index);
    view->scrollTo(index);
}

}
