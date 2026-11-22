# Technical Notes — Windows Implementation

This document records implementation guidance, not permission to assume every Windows dialog behaves identically. Verify behavior with a working prototype.

## 1. Recommended stack

- C++20.
- MSVC (Visual Studio toolchain).
- Win32 API.
- Windows SDK.
- Windows UI Automation (UIA) where appropriate.
- WinEvent hooks for event-driven window observation.
- Windows Shell APIs for known folders and path utilities.
- Native notification area icon via `Shell_NotifyIcon`.

Avoid third-party libraries unless a concrete need is demonstrated.

## 2. Dialog detection

Investigate WinEvent hooks such as relevant object/window creation, show, and location-change events. Use out-of-context hooks where appropriate and filter events carefully.

Do not assume every file dialog is a top-level window with the same class name or UI tree.

Potential dialog families include:
- Common Item Dialog (`IFileDialog` family).
- Legacy common dialogs (`GetOpenFileName` / `GetSaveFileName` family).
- Folder selection dialogs.
- Application-specific custom dialogs.

Identify and document each family separately. Avoid false positives on unrelated windows.

Use UI Automation to inspect the candidate dialog and determine whether it exposes expected controls and semantics. Cache references only while valid; handle element invalidation and dialog destruction.

## 3. Navigating the existing dialog

This is the core technical risk.

The selected folder must become the current folder in the existing host dialog. Do not:
- Open Explorer.
- Open a replacement dialog.
- Call `IFileDialog::SetFolder` on an unrelated dialog instance and assume it affects the host dialog.
- Send blind keystrokes without validating focus and target context.

Investigate UI Automation patterns and the actual control tree for each supported dialog family. Determine a reliable method to set the location and confirm navigation.

For dialogs where UIA cannot reliably set the location, evaluate a narrowly scoped fallback only if it is safe and testable. Document limitations rather than claiming universal support.

## 4. Process integrity and security boundaries

Windows UI automation and window messaging can be restricted across integrity levels. An unelevated process may not control an elevated application's UI.

Do not attempt to bypass UAC or security boundaries. Fail gracefully and document that elevated host applications may be unsupported unless the utility is run at a compatible integrity level.

Do not inject code into target processes or patch their memory.

## 5. System tray implementation

Use `Shell_NotifyIcon` with a hidden/message-only or otherwise minimal application window to receive tray callbacks and maintain the message loop.

A tiny hidden Win32 window is acceptable and is not a user-facing main GUI.

Requirements:
- Add icon at startup.
- Handle right-click/context menu.
- Handle About.
- Handle Exit.
- Remove icon on shutdown.
- Handle Explorer restart if practical (e.g. `TaskbarCreated` message), without adding a polling loop.

## 6. Overlay window

Use a small native Win32 window. Keep it owned/associated with the relevant dialog where appropriate, but verify activation, z-order, focus, and keyboard behavior experimentally.

Consider:
- Dialog movement.
- Dialog destruction.
- Multiple monitors.
- DPI awareness and coordinate conversion.
- Owner window activation.
- Focus return after Escape or selection.
- Preventing overlay interaction from disrupting the host dialog.

Do not use layered/translucent effects unless they provide a clear benefit.

## 7. Folder lookup

Use known-folder APIs to obtain standard user locations. Add a small, explicit set of sensible roots.

Search on demand. Avoid indexing all of `C:\` or removable/network drives. Handle access-denied and disconnected locations without hanging.

If asynchronous lookup is used:
- Cancel or ignore stale queries.
- Bound the number of results.
- Keep UI updates on the UI thread.
- Avoid creating a large thread pool.

## 8. Executable and dependencies

Output name must be exactly:

`thelastfolder.exe`

Build x64 Release.

Use `/MT` if it works cleanly with the chosen implementation, but correctness takes priority. Report actual dependencies and binary size. Do not claim the EXE is fully self-contained until verified on a clean Windows environment.

No installer is required.

## 9. Suggested module boundaries

Keep modules few and only split when it improves clarity:

- `App`: lifecycle and message loop.
- `TrayIcon`: notification icon and context menu.
- `DialogWatcher`: event-driven detection.
- `DialogAdapter`: inspect and navigate a supported dialog family.
- `QuickSwitchOverlay`: temporary search UI.
- `FolderLookup`: on-demand lookup.
- `KnownFolders`: known-folder roots and path helpers.

Do not create abstractions for hypothetical future features.
