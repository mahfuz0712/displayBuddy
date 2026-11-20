#include "autostart.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>

namespace Autostart {

namespace {

QString exePath() {
    return QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
}

#ifdef Q_OS_WIN
const char* kRunKey = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const char* kValueName = "DisplayBuddy";

QString command() { return QStringLiteral("\"%1\" --minimized").arg(exePath()); }
#else
QString desktopFilePath() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart";
    return dir + "/displaybuddy.desktop";
}
#endif

}  // namespace

bool isEnabled() {
#ifdef Q_OS_WIN
    QSettings run(kRunKey, QSettings::NativeFormat);
    return run.contains(kValueName);
#else
    return QFile::exists(desktopFilePath());
#endif
}

bool setEnabled(bool enabled) {
#ifdef Q_OS_WIN
    QSettings run(kRunKey, QSettings::NativeFormat);
    if (enabled) run.setValue(kValueName, command());
    else run.remove(kValueName);
    run.sync();
    return run.status() == QSettings::NoError;
#else
    const QString path = desktopFilePath();
    if (!enabled) return !QFile::exists(path) || QFile::remove(path);

    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return false;
    QTextStream out(&f);
    out << "[Desktop Entry]\n"
        << "Type=Application\n"
        << "Name=DisplayBuddy\n"
        << "Comment=Control the brightness of all your displays\n"
        << "Exec=\"" << exePath() << "\" --minimized\n"
        << "Icon=displaybuddy\n"
        << "Terminal=false\n"
        << "X-GNOME-Autostart-enabled=true\n";
    return true;
#endif
}

}  // namespace Autostart