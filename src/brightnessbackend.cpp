#include "brightnessbackend.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <algorithm>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <physicalmonitorenumerationapi.h>
#include <highlevelmonitorconfigurationapi.h>
#include <dxva2api.h>

namespace {
struct WinEnumContext {
    QVector<DisplayInfo>* result;
};

static BOOL CALLBACK enumerateWindowsMonitor(HMONITOR monitor, HDC, LPRECT, LPARAM data) {
    auto* result = reinterpret_cast<WinEnumContext*>(data)->result;

    MONITORINFOEXW info{};
    info.cbSize = sizeof(info);
    QString deviceName;
    if (GetMonitorInfoW(monitor, &info)) {
        deviceName = QString::fromWCharArray(info.szDevice);
    }

    DWORD count = 0;
    if (!GetNumberOfPhysicalMonitorsFromHMONITOR(monitor, &count) || count == 0) {
        // Keep the connected display visible even if Windows cannot expose a
        // DDC/CI physical monitor handle for it.
        DisplayInfo d;
        d.id = "display:" + deviceName;
        d.name = deviceName.isEmpty()
            ? QStringLiteral("Connected display")
            : QStringLiteral("%1 (connected; brightness API unavailable)").arg(deviceName);
        d.controllable = false;
        result->push_back(d);
        return TRUE;
    }

    QVector<PHYSICAL_MONITOR> physical(static_cast<qsizetype>(count));
    if (!GetPhysicalMonitorsFromHMONITOR(monitor, count, physical.data())) {
        DisplayInfo d;
        d.id = "display:" + deviceName;
        d.name = deviceName.isEmpty()
            ? QStringLiteral("Connected display")
            : QStringLiteral("%1 (connected; physical monitor query failed)").arg(deviceName);
        d.controllable = false;
        result->push_back(d);
        return TRUE;
    }

    for (DWORD i = 0; i < count; ++i) {
        DisplayInfo d;
        d.id = QString("ddc:%1:%2")
                   .arg(reinterpret_cast<quintptr>(monitor), 0, 16)
                   .arg(i);

        d.name = QString::fromWCharArray(physical[i].szPhysicalMonitorDescription).trimmed();
        if (d.name.isEmpty()) {
            d.name = deviceName.isEmpty()
                ? QString("External monitor %1").arg(result->size() + 1)
                : deviceName;
        }

        DWORD minB = 0, curB = 50, maxB = 100;
        if (GetMonitorBrightness(physical[i].hPhysicalMonitor, &minB, &curB, &maxB)) {
            d.minimum = static_cast<int>(minB);
            d.maximum = static_cast<int>(maxB);
            d.brightness = (maxB > minB)
                ? qRound((curB - minB) * 100.0 / (maxB - minB))
                : 50;
            d.controllable = true;
        } else {
            // Detection and brightness capability are separate. Do not hide
            // a connected HDMI/DP monitor just because it lacks VCP 0x10.
            d.controllable = false;
            d.name += QStringLiteral(" (brightness unsupported)");
        }

        result->push_back(d);
        DestroyPhysicalMonitor(physical[i].hPhysicalMonitor);
    }
    return TRUE;
}
}

static QVector<DisplayInfo> windowsDisplays() {
    QVector<DisplayInfo> result;
    WinEnumContext context{&result};
    EnumDisplayMonitors(nullptr, nullptr, enumerateWindowsMonitor,
                        reinterpret_cast<LPARAM>(&context));

    // Internal laptop panel brightness is commonly exposed through WMI.
    QProcess p;
    p.start("powershell.exe", {
        "-NoProfile", "-NonInteractive", "-Command",
        "$b=Get-CimInstance -Namespace root/WMI -ClassName WmiMonitorBrightness -ErrorAction SilentlyContinue | Select-Object -First 1; if($b){'laptop|Internal display|' + $b.CurrentBrightness}"
    });
    if (p.waitForFinished(4000) && p.exitCode() == 0) {
        const QString line = QString::fromLocal8Bit(p.readAllStandardOutput()).trimmed();
        if (line.startsWith("laptop|")) {
            const auto parts = line.split('|');
            DisplayInfo d;
            d.id = "wmi:internal";
            d.name = parts.value(1, "Internal display");
            d.brightness = parts.value(2, "50").toInt();
            d.controllable = true;

            // Avoid adding a second copy if the internal panel was already
            // exposed as a physical monitor by the graphics driver.
            bool duplicate = false;
            for (const auto& existing : result) {
                if (existing.name.contains("Internal", Qt::CaseInsensitive) ||
                    existing.name.contains("Built-in", Qt::CaseInsensitive)) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate) result.prepend(d);
        }
    }
    return result;
}
#endif

#ifdef Q_OS_LINUX
static QVector<DisplayInfo> linuxDisplays() {
    QVector<DisplayInfo> result;

    QDir dir("/sys/class/backlight");
    const auto entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& entry : entries) {
        const QString base = dir.filePath(entry);
        QFile maxFile(base + "/max_brightness");
        QFile curFile(base + "/brightness");
        if (!maxFile.open(QIODevice::ReadOnly) || !curFile.open(QIODevice::ReadOnly))
            continue;

        const int maximum = QString::fromLatin1(maxFile.readAll()).trimmed().toInt();
        const int current = QString::fromLatin1(curFile.readAll()).trimmed().toInt();
        if (maximum <= 0) continue;

        DisplayInfo d;
        d.id = "sysfs:" + entry;
        d.name = entry + " (built-in)";
        d.brightness = qRound(current * 100.0 / maximum);
        d.controllable = true;
        result.push_back(d);
    }

    // External monitors on Linux are discovered through ddcutil.
    QProcess detect;
    detect.start("ddcutil", {"detect", "--brief"});
    if (detect.waitForFinished(7000) && detect.exitCode() == 0) {
        const QString output = QString::fromLocal8Bit(detect.readAllStandardOutput());
        const auto lines = output.split('\n');
        for (const QString& line : lines) {
            const auto match = QRegularExpression("^Display\\s+(\\d+)")
                                   .match(line.trimmed());
            if (!match.hasMatch()) continue;

            const int displayNo = match.captured(1).toInt();
            QProcess get;
            get.start("ddcutil", {
                "getvcp", "10", "--display", QString::number(displayNo), "--terse"
            });

            DisplayInfo d;
            d.id = "ddc:" + QString::number(displayNo);
            d.name = QString("External monitor %1 (DDC/CI)").arg(displayNo);
            d.controllable = false;

            if (get.waitForFinished(7000) && get.exitCode() == 0) {
                const QString response = QString::fromLocal8Bit(get.readAllStandardOutput());
                const auto nums = QRegularExpression(R"(\bVCP\s+10\s+.*?(\d+)\s+(\d+))")
                                      .match(response);
                if (nums.hasMatch()) {
                    d.brightness = nums.captured(1).toInt();
                    d.controllable = true;
                } else {
                    const auto fallback = QRegularExpression(R"(\b(\d+)\s+(\d+)\b)")
                                              .match(response);
                    if (fallback.hasMatch()) {
                        d.brightness = fallback.captured(1).toInt();
                        d.controllable = true;
                    }
                }
            }
            if (!d.controllable) d.name += " (brightness unsupported)";
            result.push_back(d);
        }
    }
    return result;
}
#endif

QVector<DisplayInfo> BrightnessBackend::displays() {
#ifdef Q_OS_WIN
    return windowsDisplays();
#elif defined(Q_OS_LINUX)
    return linuxDisplays();
#else
    return {};
#endif
}

bool BrightnessBackend::setBrightness(const QString& id, int value, QString* error) {
    value = std::clamp(value, 0, 100);

#ifdef Q_OS_WIN
    if (id == "wmi:internal") {
        QProcess p;
        p.start("powershell.exe", {
            "-NoProfile", "-NonInteractive", "-Command",
            QString("(Get-CimInstance -Namespace root/WMI -ClassName WmiMonitorBrightnessMethods | Select-Object -First 1).WmiSetBrightness(1,%1)").arg(value)
        });
        if (!p.waitForFinished(7000) || p.exitCode() != 0) {
            if (error) *error = "Windows could not change the internal panel brightness.";
            return false;
        }
        return true;
    }

    if (id.startsWith("ddc:")) {
        const auto bits = id.mid(4).split(':');
        bool ok = false;
        const quintptr handle = bits.value(0).toULongLong(&ok, 16);
        const int index = bits.value(1).toInt();
        if (!ok) {
            if (error) *error = "Invalid physical monitor identifier.";
            return false;
        }

        const HMONITOR monitor = reinterpret_cast<HMONITOR>(handle);
        DWORD count = 0;
        if (!GetNumberOfPhysicalMonitorsFromHMONITOR(monitor, &count)) {
            if (error) *error = "Windows could not reopen this monitor. Refresh the display list.";
            return false;
        }

        QVector<PHYSICAL_MONITOR> physical(static_cast<qsizetype>(count));
        if (!GetPhysicalMonitorsFromHMONITOR(monitor, count, physical.data())) {
            if (error) *error = "Windows could not access the monitor's DDC/CI interface.";
            return false;
        }

        bool success = false;
        if (index >= 0 && static_cast<DWORD>(index) < count) {
            DWORD minB = 0, curB = 0, maxB = 100;
            if (GetMonitorBrightness(physical[index].hPhysicalMonitor, &minB, &curB, &maxB)) {
                const DWORD target = minB + static_cast<DWORD>(
                    (maxB - minB) * value / 100.0);
                success = SetMonitorBrightness(physical[index].hPhysicalMonitor, target);
            }
        }
        for (const auto& monitorInfo : physical)
            DestroyPhysicalMonitor(monitorInfo.hPhysicalMonitor);

        if (!success && error)
            *error = "This monitor was detected, but it does not expose DDC/CI brightness control. Enable DDC/CI in the monitor's OSD menu and check its HDMI cable/adapter.";
        return success;
    }

    if (id.startsWith("display:")) {
        if (error) *error = "Windows detected this display, but its driver did not expose a controllable DDC/CI monitor. Enable DDC/CI in the monitor menu.";
        return false;
    }

#elif defined(Q_OS_LINUX)
    if (id.startsWith("sysfs:")) {
        const QString base = "/sys/class/backlight/" + id.mid(6);
        QFile maxFile(base + "/max_brightness");
        if (!maxFile.open(QIODevice::ReadOnly)) {
            if (error) *error = "Cannot read the backlight maximum.";
            return false;
        }
        const int maximum = QString::fromLatin1(maxFile.readAll()).trimmed().toInt();
        QFile file(base + "/brightness");
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            if (error) *error = "Permission denied writing /sys/class/backlight/brightness.";
            return false;
        }
        const QByteArray bytes = QByteArray::number(qRound(maximum * value / 100.0));
        if (file.write(bytes) != bytes.size()) {
            if (error) *error = "Failed to write the brightness value.";
            return false;
        }
        return true;
    }

    if (id.startsWith("ddc:")) {
        QProcess p;
        p.start("ddcutil", {
            "setvcp", "10", QString::number(value),
            "--display", id.mid(4)
        });
        if (!p.waitForFinished(10000) || p.exitCode() != 0) {
            if (error) *error = "ddcutil failed. Check DDC/CI and /dev/i2c-* permissions.";
            return false;
        }
        return true;
    }
#endif

    if (error) *error = "This display is not controllable by the current platform backend.";
    return false;
}
