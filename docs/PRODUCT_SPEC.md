# Product Specification — The Last Folder

> **Scope change (requested by the product owner, 2026-11-22). This overrides sections 1, 5, 6 and 7 below.**
> The product has **no overlay, no search box and no hotkey**. It is fully automatic:
> 1. The app remembers the folder of the file manager the user used last (**Windows Explorer** and **Total Commander**).
> 2. When a standard Open / Save As dialog of another program becomes active, it jumps to that folder by itself.
> 3. The user only has to use their file manager and then go back to the dialog. Nothing is typed or invoked.
>
> Still in force: tray icon with About/Exit, no settings, no main window, event-driven and near-idle, never guess
> (if the folder is unknown, do nothing), fail safely. Details of the implemented behavior are in `STATUS.md`.
> The sections below are kept as originally written and are partly superseded.
>
> **Second change (requested by the product owner, 2026-11-24): "Run at Startup".** The tray menu is now **Run at Startup**
> (a checkable item), a separator, **About** and **Exit**. This overrides section 4 ("About, Exit") and removes
> "Automatic start with Windows" from the exclusions in section 3. It is the standard per-user `Run` registry value
> (`HKCU\Software\Microsoft\Windows\CurrentVersion\Run`, name `TheLastFolder`); no settings file or other persistent
> configuration is introduced, and the check mark is always read back from the registry.
>
> **Third change (requested by the product owner, 2026-11-26): "Set Icon".** The tray menu is now **Run at Startup**,
> **Set Icon**, a separator, **About**, **Exit**. "Set Icon" opens one small window: a check box "Set custom icon", a
> "Browse" button with the chosen file shown to its right, and "Apply" / "Cancel". Unticking and applying returns to the
> built-in icon but keeps the chosen path so it can be switched on again. This needs the choice to survive a restart, so
> two values are stored in the registry (`HKCU\Software\TheLastFolder`: `CustomIconEnabled`, `CustomIconPath`). It overrides
> the earlier "no settings" rule only to that extent; the program still creates no file of its own.

## 1. Product concept

The Last Folder is a very small Windows utility that does exactly one thing: it switches the folder of an already-open file dialog.

Its sole purpose is to let a user quickly change the current folder inside an already-open Windows file dialog.

Example workflow:

1. The user opens **Save As** in an application.
2. The Last Folder detects a supported dialog.
3. The user invokes Quick Switch using the agreed activation method.
4. A small search overlay appears.
5. The user types a folder name or path.
6. Matching folders appear.
7. The user selects a result.
8. The existing Save As dialog navigates to that folder.
9. The user saves the file normally.

It must not open File Explorer as a substitute, launch a second file dialog, or replace the host application's dialog.

## 2. Product identity

- Name: **The Last Folder**
- Executable: **`thelastfolder.exe`**
- Internal name: `TheLastFolder`

## 3. Strict scope

### Included

- Background process.
- System tray icon indicating that the process is running.
- Native right-click tray menu.
- Detection of supported file dialogs.
- Temporary Quick Switch overlay.
- Folder-name and full-path lookup.
- Keyboard and mouse selection.
- Navigation of the existing file dialog.
- Graceful cleanup on exit.

### Explicitly excluded for the initial release

- Global file search.
- File indexing or database.
- Permanent filesystem scanning.
- Settings or preferences.
- Configuration files.
- Main application window.
- System tray notifications.
- Installer.
- Auto-updater.
- Telemetry.
- Cloud features.
- File previews or file management.
- Application launcher.
- Automatic start with Windows.
- Per-application settings.
- Custom themes.

## 4. System tray

Use the native Win32 notification area mechanism (`Shell_NotifyIcon`).

The tray icon must:
- Be visible while the application is running.
- Have a recognizable folder-related design.
- Have a tooltip identifying **The Last Folder**.
- Support right-click.

Initial context menu:
1. About
2. Exit

About may show a small native message/dialog containing the product name and version. Exit must remove the tray icon, unregister hooks, release resources, and terminate cleanly.

Keep menu construction modular so commands can be added later. Do not implement future commands now.

## 5. Quick Switch overlay

The overlay is the only operational UI.

It should be:
- Small and compact.
- Visually simple.
- Native-looking.
- Positioned near the detected dialog where practical.
- Correctly placed across multiple monitors and DPI scales.
- Dismissible with Escape.
- Hidden when its associated dialog closes or becomes invalid.

Minimum contents:
- One search input.
- A short result list.
- Folder name.
- Full path as secondary information.

Do not add toolbars, settings, tabs, navigation panes, or decorative UI.

## 6. Activation and keyboard behavior

The desired user workflow should be as quick and unobtrusive as practical.

Investigate a suitable activation approach for the first prototype. A global `Ctrl+G` hotkey is a reasonable initial default, but verify conflicts and behavior before committing to it.

Overlay keyboard behavior:
- Up / Down: move selection.
- Enter: select/navigate to the highlighted folder.
- Escape: close the overlay.
- Typing: update the folder query.
- Pasting a full path: allow direct navigation if it is a valid folder.

Mouse:
- Click a result to select/navigate.
- Double-click behavior may be used if it is consistent and documented.

Do not steal keyboard input from unrelated applications when no supported dialog is active.

## 7. Folder lookup

Keep folder lookup simple and on-demand.

- Search folder names, not file contents.
- Support case-insensitive partial matches.
- Rank exact and prefix matches above weaker matches.
- Show a small capped result list (approximately 8 items).
- Support direct full-path input.
- Do not recursively scan all drives on every keystroke.
- Do not maintain a permanent index or database.

Start with sensible known user locations and a deliberately limited set of roots. The implementation should avoid blocking the UI. If asynchronous work is needed, use the smallest practical worker mechanism and cancel stale searches.

The exact default roots should be documented in code and must not require a settings file.

## 8. Performance and resource principles

The app should be almost idle when unused.

- Event-driven detection; no tight polling loops.
- No continuous drive scanning.
- No permanent folder index.
- No unnecessary threads.
- No repeated UI Automation queries without a reason.
- No unnecessary allocations.
- Release COM interfaces, handles, hooks, and windows correctly.
- Search only while the overlay is active.
- Do not perform expensive filesystem work on the UI thread.

Do not optimize blindly. Correctness and reliability come first; measure idle CPU and memory after the prototype works.

## 9. Compatibility expectations

Initial target:
- Windows 10 x64.
- Windows 11 x64.
- Standard Windows common file dialogs.
- Standard folder selection dialogs where feasible.

Some applications implement custom dialogs. These may not be supported initially.

The app must fail safely when:
- A dialog cannot be recognized.
- UI Automation cannot access its controls.
- The dialog closes during an operation.
- A folder no longer exists.
- A drive is disconnected.
- The target application is elevated and access is restricted.

Never crash or destabilize the host application.
