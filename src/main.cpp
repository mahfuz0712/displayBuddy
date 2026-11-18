#include "mainwindow.h"
#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("DisplayBuddy");
    QApplication::setOrganizationName("DisplayBuddy");
    QApplication::setStyle("Fusion");
    MainWindow window;
    window.show();
    return app.exec();
}
