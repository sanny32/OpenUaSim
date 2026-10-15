// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file statcardwidget.h
/// \brief Declares the quick statistics card.
///

#pragma once

#include <QColor>
#include <QFrame>

#include "data/statuskind.h"

namespace Ui {
class StatCardWidget;
}

///
/// \brief Card showing a coloured icon, a caption and a prominent value.
///
class StatCardWidget : public QFrame
{
    Q_OBJECT

public:
    ///
    /// \brief Builds an empty card.
    /// \param parent Parent widget.
    ///
    explicit StatCardWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the card.
    ///
    ~StatCardWidget() override;

    ///
    /// \brief Sets the card icon.
    /// \param name Icon resource name.
    /// \param color Colour the icon is drawn in.
    ///
    void setIcon(const QString &name, const QColor &color);

    ///
    /// \brief Sets the caption above the value.
    /// \param caption Caption text.
    ///
    void setCaption(const QString &caption);

    ///
    /// \brief Sets the value.
    /// \param value Value text.
    /// \param status Status colouring the value; StatusKind::None keeps the text colour.
    ///
    void setValue(const QString &value, StatusKind status = StatusKind::None);

    ///
    /// \brief Returns the value text.
    /// \return Value text.
    ///
    QString value() const;

    ///
    /// \brief Shows or hides the chevron hinting that the card leads to a page.
    /// \param visible True to show the chevron.
    ///
    void setChevronVisible(bool visible);

protected:
    void changeEvent(QEvent *event) override;

private:
    void refreshPixmaps();

    Ui::StatCardWidget *ui;
    QString _iconName;
    QColor _iconColor;
    StatusKind _valueStatus = StatusKind::None;
};
