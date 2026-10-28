// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#import <AppKit/AppKit.h>

#include <QWidget>

///
/// \brief Checks native window decorations and excludes sheet presentation.
/// \param widget Visible Cocoa dialog.
/// \return Whether the dialog has a title bar with only the close button.
///
bool isTitledMacDialog(QWidget *widget)
{
    NSView *view = reinterpret_cast<NSView *>(widget->winId());
    NSWindow *window = view.window;
    return window && !window.sheetParent
        && (window.styleMask & NSWindowStyleMaskTitled)
        && window.title.length > 0
        && ![window standardWindowButton:NSWindowCloseButton].hidden
        && [window standardWindowButton:NSWindowMiniaturizeButton].hidden
        && [window standardWindowButton:NSWindowZoomButton].hidden;
}
