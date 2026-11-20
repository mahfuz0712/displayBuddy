#pragma once

#include <QColor>
#include <QPalette>
#include <QString>

// Windows 11 (Fluent) inspired light/dark colour sets with a warm amber accent.
struct Theme {
    bool dark = false;
    QColor bg, card, cardBorder, text, subtext;
    QColor control, controlHover, controlBorder, track;
    QColor accent, accentSoft, accentText, danger;

    static bool systemIsDark();
    static void captureSystemScheme();   // call once before QApplication::setStyle()
    static Theme make(bool dark);

    QString styleSheet() const;
    QPalette palette() const;
};
