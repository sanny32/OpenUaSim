// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file texttooltip.h
/// \brief Declares full-value tooltips shared by text controls and item views.
///

#pragma once

#include <QRect>
#include <QString>

class QHelpEvent;
class QWidget;

namespace TextToolTip {

///
/// \brief Shows plain text with preserved whitespace and wrapping at the screen edge.
/// \param event Tooltip request supplying the global position.
/// \param widget Widget owning the tooltip.
/// \param text Complete value to display.
/// \param rect Area outside which the tooltip hides; empty uses the whole widget.
///
void show(const QHelpEvent *event, QWidget *widget, const QString &text, const QRect &rect = {});

}
