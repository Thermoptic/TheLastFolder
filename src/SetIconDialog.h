// SetIconDialog: the modal "Set Icon" window. A check box "Set custom icon", a Browse button with the chosen file shown to
// its right, and Apply / Cancel. Unticking the box means "use the built-in icon" but keeps the chosen file, so ticking it
// again brings the same file back. Apply saves the choice (see Settings); Cancel changes nothing.
#pragma once
#include <windows.h>

// Returns true if the user pressed Apply and the choice was saved; the caller then applies it to the tray icon.
bool ShowSetIconDialog(HINSTANCE inst, HWND owner);
