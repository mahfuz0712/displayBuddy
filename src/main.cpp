#include "icons.h"
#include "mainwindow.h"
#include "theme.h"

#include <QApplication>
#include <QFont>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("DisplayBuddy");
    QApplication::setOrganizationName("DisplayBuddy");
    QApplication::setApplicationVersion(DISPLAYBUDDY_VERSION);
    QGuiApplication::setDesktopFileName("displaybuddy");   // Linux: matches displaybuddy.desktop

    Theme::captureSystemScheme();       // read the OS scheme before Fusion replaces the palette
    QApplication::setStyle("Fusion");

    QFont font = app.font();
    font.setFamilies({"Segoe UI Variable Text", "Segoe UI", "Noto Sans", "Ubuntu", "Cantarell", "DejaVu Sans"});
    font.setPointSizeF(9.5);
    app.setFont(font);

    app.setWindowIcon(Icons::appIcon());   // title bar + taskbar/dock

    MainWindow window;
    window.show();
    return app.exec();
}
