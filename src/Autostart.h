// Autostart: start this program when the user logs in, using the standard per-user mechanism (the "Run" registry key of
// the current user). Nothing is written outside that key, no admin rights are needed, and no file of our own is created.
// The registry is the single source of truth, so the menu check mark always shows what Windows will actually do.
#pragma once
#include <windows.h>

namespace Autostart {

// The registry value name. Tests may pass another name so they never touch the real entry.
inline constexpr wchar_t kDefaultName[] = L"TheLastFolder";

// True if this exact exe is registered to start at login AND Windows has not disabled it (Task Manager's Startup tab and
// Settings > Apps > Startup record their off switch separately, in "StartupApproved").
bool IsEnabled(const wchar_t* name = kDefaultName);

// Turns it on or off. Turning it on also clears a "disabled" mark Windows may have recorded for the entry. Returns false
// if the registry could not be written (for example blocked by policy).
bool SetEnabled(bool enable, const wchar_t* name = kDefaultName);

}  // namespace Autostart
