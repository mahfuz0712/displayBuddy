#pragma once

// "Start with Windows" (registry Run key) / "Start at login" (~/.config/autostart).
// The app is started with --minimized so it only appears in the system tray.
namespace Autostart {

bool isEnabled();
bool setEnabled(bool enabled);

}  // namespace Autostart