#include "icons.h"
#include "mainwindow.h"
#include "theme.h"

#include <QApplication>
#include <QFont>
#include <QLocalServer>
#include <QLocalSocket>

namespace {

// One socket name per OS user, so two people signed in at the same time (fast user switching)
// each get their own DisplayBuddy.
QString instanceName() {
    QString user = qEnvironmentVariable("USERNAME");
    if (user.isEmpty()) user = qEnvironmentVariable("USER");
    return QStringLiteral("DisplayBuddy-") + user;
}

}  // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("DisplayBuddy");
    QApplication::setOrganizationName("DisplayBuddy");
    QApplication::setApplicationVersion(DISPLAYBUDDY_VERSION);
    QGuiApplication::setDesktopFileName("displaybuddy");   // Linux: matches displaybuddy.desktop

    // Closing the window only hides it to the system tray; "Quit" in the tray menu really exits.
    QApplication::setQuitOnLastWindowClosed(false);

    const bool minimized = QCoreApplication::arguments().contains(QStringLiteral("--minimized"));

    // ---- single instance -------------------------------------------------------------------
    // If DisplayBuddy is already running (e.g. it started with Windows and sits in the tray),
    // ask that copy to show its window and exit. A "--minimized" start never pops a window up.
    const QString name = instanceName();
    {
        QLocalSocket probe;
        probe.connectToServer(name);
        if (probe.waitForConnected(500)) {
            if (!minimized) {
                probe.write("show");
                probe.waitForBytesWritten(1000);
            }
            probe.disconnectFromServer();
            return 0;
        }
    }
    QLocalServer::removeServer(name);   // clean up a stale socket left by a crash (Linux)
    QLocalServer server;
    server.listen(name);

    Theme::captureSystemScheme();       // read the OS scheme before Fusion replaces the palette
    QApplication::setStyle("Fusion");

    QFont font = app.font();
    font.setFamilies({"Segoe UI Variable Text", "Segoe UI", "Noto Sans", "Ubuntu", "Cantarell", "DejaVu Sans"});
    font.setPointSizeF(9.5);
    app.setFont(font);

    app.setWindowIcon(Icons::appIcon());   // title bar + taskbar/dock

    MainWindow window;

    QObject::connect(&server, &QLocalServer::newConnection, &window, [&server, &window] {
        while (QLocalSocket* client = server.nextPendingConnection()) {
            QObject::connect(client, &QLocalSocket::readyRead, &window, [client, &window] {
                if (client->readAll().contains("show")) window.showFromTray();
                client->disconnectFromServer();
            });
            QObject::connect(client, &QLocalSocket::disconnected, client, &QObject::deleteLater);
        }
    });

    // Start hidden in the tray when launched with --minimized (Windows startup entry).
    // Without a tray (some Linux desktops) the window is shown, otherwise the app would be invisible.
    if (minimized && window.hasTray()) {
        // tray icon only
    } else {
        window.show();
    }

    return app.exec();
}