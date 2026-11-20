#pragma once

#include <QColor>
#include <QIcon>
#include <QPixmap>

// Fluent-style outline icons drawn with QPainter, so they stay crisp at any
// DPI and follow the light/dark theme colours (no extra Qt modules needed).
namespace Icons {

enum class Kind { Gear, Refresh, Monitor, Sun, Plus, Trash };

QPixmap pixmap(Kind kind, const QColor& color, int logicalSize, qreal dpr = 1.0);
QIcon icon(Kind kind, const QColor& color);

// DisplayBuddy application icon (multi-size, from the Qt resource file).
QIcon appIcon();

}  // namespace Icons
