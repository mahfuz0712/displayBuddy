# DisplayBuddy 1.2

DisplayBuddy is a compact C++20 display brightness controller with a Windows 11-inspired interface.

## UI and behavior

- DisplayBuddy branding; no developer/toolchain footer in the app.
- Monitor icon and sun/brightness iconography.
- All detected displays appear as separate cards.
- Three quick presets: Designing (85%), Coding (65%), Editing (95%).
- Custom presets can be saved by name, appear as quick-access buttons, and are persisted with QSettings. Custom presets can be deleted from the settings menu.
- No Apply button: brightness is changed when the slider is released.
- Refresh connected displays using the refresh icon.
- Displays detected without brightness capability remain visible but have disabled controls.

## Build: Windows MSYS2 UCRT64

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-qt6-base
cmake -S . -B build-windows -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-windows --parallel
./build-windows/brightness-controller.exe
```

## Build: Ubuntu

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build qt6-base-dev ddcutil i2c-tools
cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux --parallel
./build-linux/brightness-controller
```

## External monitors

DDC/CI must be enabled in the monitor's on-screen menu. Windows external monitor control uses DXVA2; Linux uses `ddcutil`. Some HDMI adapters, docks and monitors do not expose DDC/CI brightness.

Do not reuse a CMake build directory between Windows and WSL/Linux.
