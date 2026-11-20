#include "theme.h"

#include <QGuiApplication>
#include <QStyleHints>
#include <QtGlobal>

namespace {
bool g_startupDark = false;
}

void Theme::captureSystemScheme() {
    g_startupDark = QGuiApplication::palette().color(QPalette::Window).lightness() < 128;
}

bool Theme::systemIsDark() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    const Qt::ColorScheme scheme = QGuiApplication::styleHints()->colorScheme();
    if (scheme != Qt::ColorScheme::Unknown)
        return scheme == Qt::ColorScheme::Dark;
#endif
    return g_startupDark;
}

Theme Theme::make(bool dark) {
    Theme t;
    t.dark = dark;
    if (dark) {
        t.bg = "#202020";
        t.card = "#2b2b2b";
        t.cardBorder = "#3a3a3a";
        t.text = "#f3f3f3";
        t.subtext = "#a8a8a8";
        t.control = "#323232";
        t.controlHover = "#3b3b3b";
        t.controlBorder = "#454545";
        t.track = "#5c5c5c";
        t.accent = "#ffb02e";
        t.accentSoft = "#3a2f1a";
        t.accentText = "#1b1b1b";
        t.danger = "#ff99a4";
    } else {
        t.bg = "#f3f3f3";
        t.card = "#ffffff";
        t.cardBorder = "#e3e3e3";
        t.text = "#1b1b1b";
        t.subtext = "#616161";
        t.control = "#ffffff";
        t.controlHover = "#f6f6f6";
        t.controlBorder = "#dcdcdc";
        t.track = "#c9c9c9";
        t.accent = "#d97706";
        t.accentSoft = "#fff4e0";
        t.accentText = "#ffffff";
        t.danger = "#c42b1c";
    }
    return t;
}

QPalette Theme::palette() const {
    QPalette p;
    p.setColor(QPalette::Window, bg);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, control);
    p.setColor(QPalette::AlternateBase, card);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, control);
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::ToolTipBase, card);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::PlaceholderText, subtext);
    p.setColor(QPalette::Highlight, accent);
    p.setColor(QPalette::HighlightedText, accentText);
    p.setColor(QPalette::Link, accent);
    p.setColor(QPalette::Disabled, QPalette::Text, subtext);
    p.setColor(QPalette::Disabled, QPalette::WindowText, subtext);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, subtext);
    return p;
}

QString Theme::styleSheet() const {
    QString s = QStringLiteral(R"(
QMainWindow, QDialog, QWidget#central { background: @bg@; }
QLabel { color: @text@; background: transparent; }
QLabel#title { font-size: 13pt; font-weight: 600; }
QLabel#sectionTitle { font-size: 10pt; font-weight: 600; }
QLabel#muted { color: @subtext@; font-size: 9pt; }
QLabel#brightnessValue { color: @accent@; font-weight: 600; }
QLabel#status { color: @subtext@; font-size: 9pt; }
QLabel#status[error="true"] { color: @danger@; }
QLabel#presetName { font-weight: 600; }
QLabel#presetValue { color: @subtext@; font-size: 9pt; }

QFrame#displayCard {
    background: @card@;
    border: 1px solid @cardBorder@;
    border-radius: 8px;
}
QFrame#displayCard QLabel { background: transparent; }
QFrame#separator { background: @cardBorder@; max-height: 1px; border: none; }

QPushButton#presetCard {
    background: @card@;
    border: 1px solid @cardBorder@;
    border-radius: 8px;
    text-align: left;
}
QPushButton#presetCard:hover { background: @controlHover@; }
QPushButton#presetCard:pressed { background: @control@; }
QPushButton#presetCard:focus { border: 1px solid @accent@; }
QPushButton#presetCard[active="true"] {
    background: @accentSoft@;
    border: 1px solid @accent@;
}
QPushButton#presetCard QLabel { background: transparent; }
QPushButton#presetCard[active="true"] QLabel#presetValue { color: @accent@; }

QPushButton#presetNew {
    background: transparent;
    color: @accent@;
    border: 1px dashed @controlBorder@;
    border-radius: 8px;
    padding: 0 16px;
    font-weight: 600;
}
QPushButton#presetNew:hover { background: @controlHover@; border-color: @accent@; }

QToolButton#toolButton {
    background: transparent;
    border: 1px solid transparent;
    border-radius: 6px;
}
QToolButton#toolButton:hover { background: @controlHover@; border-color: @cardBorder@; }
QToolButton#toolButton:pressed { background: @control@; }

QDialogButtonBox QPushButton {
    background: @control@;
    color: @text@;
    border: 1px solid @controlBorder@;
    border-radius: 6px;
    padding: 6px 18px;
    min-width: 64px;
}
QDialogButtonBox QPushButton:hover { background: @controlHover@; }
QDialogButtonBox QPushButton:default {
    background: @accent@;
    color: @accentText@;
    border-color: @accent@;
    font-weight: 600;
}
QDialogButtonBox QPushButton:disabled { color: @subtext@; background: @bg@; }

QLineEdit {
    background: @control@;
    color: @text@;
    border: 1px solid @controlBorder@;
    border-bottom: 2px solid @controlBorder@;
    border-radius: 6px;
    padding: 6px 8px;
    selection-background-color: @accent@;
    selection-color: @accentText@;
}
QLineEdit:focus { border-bottom: 2px solid @accent@; }

QSlider::groove:horizontal { height: 4px; background: @track@; border-radius: 2px; }
QSlider::sub-page:horizontal { background: @accent@; border-radius: 2px; }
QSlider::handle:horizontal {
    background: @accent@;
    border: 5px solid @card@;
    width: 10px;
    height: 10px;
    margin: -8px 0;
    border-radius: 10px;
}
QSlider::handle:horizontal:hover { border-width: 4px; }
QSlider::sub-page:horizontal:disabled { background: @track@; }
QSlider::handle:horizontal:disabled { background: @track@; }

QScrollArea { background: transparent; border: none; }
QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
QScrollBar::handle:vertical { background: @track@; border-radius: 3px; min-height: 28px; }
QScrollBar::handle:vertical:hover { background: @subtext@; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }

QMenu {
    background: @card@;
    color: @text@;
    border: 1px solid @cardBorder@;
    padding: 4px;
}
QMenu::item { padding: 7px 28px 7px 12px; border-radius: 4px; }
QMenu::item:selected { background: @controlHover@; }
QMenu::item:disabled { color: @subtext@; }
QMenu::separator { height: 1px; background: @cardBorder@; margin: 4px 6px; }

QToolTip {
    background: @card@;
    color: @text@;
    border: 1px solid @cardBorder@;
    padding: 4px 6px;
}
)");
    const auto put = [&s](const char* key, const QColor& c) {
        const QString token = QStringLiteral("@") + QLatin1String(key) + QStringLiteral("@");
        s.replace(token, c.name());
    };
    put("bg", bg);
    put("card", card);
    put("cardBorder", cardBorder);
    put("text", text);
    put("subtext", subtext);
    put("control", control);
    put("controlHover", controlHover);
    put("controlBorder", controlBorder);
    put("track", track);
    put("accent", accent);
    put("accentSoft", accentSoft);
    put("accentText", accentText);
    put("danger", danger);
    return s;
}
