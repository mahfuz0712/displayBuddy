# DisplayBuddy 1.3

Compact C++20 / Qt 6 display-brightness controller with a Windows 11 (Fluent) look.

## Features

- Windows 11 Fluent UI: rounded cards, Segoe UI Variable, light/dark theme that follows the OS, amber accent,
  dark title bar + rounded corners on Windows 11.
- Fluent outline icons (settings gear, refresh, monitor, sun, plus, trash) drawn with QPainter, so they are crisp at any DPI and
  recolour with the theme.
- Fully responsive: freely resizable/maximizable window (min 340 px wide); preset cards wrap onto new rows;
  display cards switch between 1, 2 or 3 columns by available width; margins shrink on narrow windows.
- DisplayBuddy app icon: `resources/icons/displaybuddy.ico` (16-256 px) for the Windows title bar, taskbar and `.exe`
  (via `resources/displaybuddy.rc.in`), plus PNG/SVG + `.desktop` file for Linux.
- Presets: Designing 85 %, Coding 65 %, Editing 95 %, and your own. Save with **New preset** (or Settings menu),
  delete with right-click on a custom preset or Settings > Delete custom preset. Stored with `QSettings`
  (presets from DisplayBuddy 1.2 are migrated automatically).
- No Apply button: brightness is sent when you release the slider, click on the track, or stop using keyboard/mouse-wheel.
- Hardware access runs on a worker thread, so the window never freezes while Windows/`ddcutil` talk to the monitors.
- Window size/position is remembered.

## Build: Windows (MSYS2 UCRT64)

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake \
                   mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-qt6-base
cmake -S . -B build-windows -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-windows --parallel
./build-windows/displaybuddy.exe
```

To run the `.exe` outside MSYS2: `windeployqt6 build-windows/displaybuddy.exe` (or copy the Qt DLLs next to it).

## Build: Ubuntu

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build qt6-base-dev ddcutil i2c-tools
cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux --parallel
./build-linux/displaybuddy
```

Optional install (adds the menu entry and hicolor icons so the launcher/taskbar shows the DisplayBuddy icon):

```bash
sudo cmake --install build-linux --prefix /usr/local
```

Do not reuse one CMake build directory between Windows and WSL/Linux.

## UI development without monitors (e.g. WSL)

```bash
cmake -S . -B build-fake -G Ninja -DDISPLAYBUDDY_FAKE_DISPLAYS=ON
cmake --build build-fake && ./build-fake/displaybuddy
```

Shows three fake displays (internal, DDC/CI, unsupported).

## Hardware notes

- External monitors need DDC/CI enabled in their on-screen menu. Windows uses DXVA2, Linux uses `ddcutil`
  (your user needs access to `/dev/i2c-*`, e.g. `sudo usermod -aG i2c $USER`).
- Laptop panels: Windows via WMI; Linux via `/sys/class/backlight` (needs write permission, e.g. a udev rule
  or the `video` group on some distros).

## Regenerating the icons

`python tools/make_icons.py` (needs `pillow`) rebuilds the `.ico` and all PNG sizes in `resources/icons/`.

## Project layout

```
CMakeLists.txt
resources/   resources.qrc, displaybuddy.rc.in, displaybuddy.desktop, icons/
src/         main, mainwindow, widgets (slider/cards/grid), flowlayout, icons, theme,
             brightnessbackend (Windows/Linux), brightnessworker (thread)
tools/       make_icons.py
```
