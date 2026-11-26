// Settings: the user's custom tray icon choice, kept in the current user's registry (HKCU\Software\TheLastFolder).
// No file of our own is created. Delete that key to forget everything.
#pragma once
#include <windows.h>
#include <string>

namespace Settings {

struct CustomIcon {
    bool enabled = false;   // use the file below instead of the built-in icon
    std::wstring path;      // the icon file the user picked; kept even when `enabled` is false, so it can be turned back on
};

CustomIcon LoadCustomIcon();
bool SaveCustomIcon(const CustomIcon& value);

}  // namespace Settings
