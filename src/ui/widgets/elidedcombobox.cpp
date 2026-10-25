// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file elidedcombobox.cpp
/// \brief Implements overflow handling for combo boxes.
///

#include <QAbstractItemView>
#include <QHelpEvent>
#include <QStylePainter>
#include <QStyleOptionComboBox>
#include <QToolTip>

#include "elidedcombobox.h"
#include "elidedlineedit.h"
#include "itemdelegate.h"
#include "texttooltip.h"

///
/// \brief Constructs an empty combo box.
/// \param parent Parent widget.
///
ElidedComboBox::ElidedComboBox(QWidget *parent)
    : QComboBox(parent)
{
    view()->setTextElideMode(Qt::ElideRight);
    view()->setItemDelegate(new ItemDelegate(view()));
}

///
/// \brief Uses an eliding line edit when manual input is enabled.
/// \param editable Whether the user may enter a custom value.
///
void ElidedComboBox::setEditable(bool editable)
{
    QComboBox::setEditable(editable);
    if (editable && !qobject_cast<ElidedLineEdit *>(lineEdit()))
        setLineEdit(new ElidedLineEdit(this));
}

///
/// \brief Tells whether the current value exceeds the available text width.
/// \return True when the value needs an ellipsis.
///
bool ElidedComboBox::isElided() const
{
    if (const auto *edit = qobject_cast<ElidedLineEdit *>(lineEdit()))
        return edit->isElided();
    return fontMetrics().horizontalAdvance(currentText()) > textWidth();
}

///
/// \brief Shows the complete clipped value unless the control has an explicit tooltip.
/// \param event Widget event.
/// \return True when the event was handled.
///
bool ElidedComboBox::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip && toolTip().isEmpty()) {
        if (isElided())
            TextToolTip::show(static_cast<QHelpEvent *>(event), this,
                              isEditable() ? lineEdit()->displayText() : currentText());
        else
            QToolTip::hideText();
        return true;
    }
    return QComboBox::event(event);
}

///
/// \brief Elides the selected value while leaving focused manual input to the line edit.
/// \param event Paint event.
///
void ElidedComboBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QStylePainter painter(this);
    QStyleOptionComboBox option;
    initStyleOption(&option);
    painter.drawComplexControl(QStyle::CC_ComboBox, option);
    if (!isEditable())
        option.currentText = fontMetrics().elidedText(currentText(), Qt::ElideRight, textWidth());
    painter.drawControl(QStyle::CE_ComboBoxLabel, option);
}

///
/// \brief Reserves the arrow, style padding and current icon before measuring the text.
/// \return Available width in pixels.
///
int ElidedComboBox::textWidth() const
{
    QStyleOptionComboBox option;
    initStyleOption(&option);
    int width = style()->subControlRect(QStyle::CC_ComboBox, &option,
                                       QStyle::SC_ComboBoxEditField, this).width() - 4;
    if (!option.currentIcon.isNull())
        width -= option.iconSize.width() + 4;
    return qMax(0, width);
}
