# Development Status

## Current stage
**v0.4.0 prototype (first release): behavior corrected after the first real test (see "Behavior" below), Double Commander
and the application icon added.** Script-tested cleanly against
test doubles (table below). **The product owner then tried it by hand and reported "it works"** (2026-11-22); which
combinations were tried (Explorer, Total Commander, Open vs Save As) was not itemized, so only treat what they named as confirmed.
Output: `src\out\thelastfolder.exe` (the temporary `out-test`/`out-v2` build folders were removed).

### Re-test of the corrected behavior (fake Total Commander + real Save/Open dialogs in another process, one run)
| Step | Expected | Observed |
|---|---|---|
| TC used, then a Save As dialog opens | stays in `C:\Windows` | stays, filename `Untitled` |
| TC -> left panel (`System32`), back to the dialog | jumps to `C:\Windows\System32` | jumped, filename kept |
| TC -> right panel (`Program Files`), back to the dialog | jumps to `C:\Program Files` | jumped, filename kept |
| A new Open dialog afterwards | stays in `C:\Windows` | stays |
An earlier run of this test gave wrong results because an older build was still running and two instances acted on the
same dialog (it even garbled the file name). Running two instances is not prevented by the app; keep to one.

## Select Folder dialogs (added 2026-11-28, after release 0.4.0; not released yet)
Reported by the product owner: the app did nothing in a "Select Folder" dialog (seen when Claude Code asked for a folder).
Cause: `DialogAdapter` deliberately ignored the folder picker (edit id 1152). That was based on an assumption written in
`docs/DESIGN_NOTE.md`, "path + OK would select the folder and close the dialog", which had **never been tested**.
- Measured against a real `IFileOpenDialog` with `FOS_PICKFOLDERS` in another process: replacing the filename box text with a
  folder path and pressing OK moves the dialog there and keeps it open (filename box emptied); pressing "Select Folder"
  afterwards returns exactly that folder. Checked for `C:\Users` and `C:\Program Files (x86)`; the host now writes the folder
  it returned to a file (`poc/dialoghost.cpp`, 3rd argument) so this could be proven and not just observed.
- Fix: removed the block in `IsSupported` and `Navigate` (`src/DialogAdapter.cpp`); the same navigation method is used.
- Verified end to end in the app (fake Total Commander, then fake Double Commander): the picker opens and stays put; after
  using the file manager and coming back it jumps to the right folder and stays open; "Select Folder" then returns that folder.
  Then 5 more runs of the Total Commander case (3 Select Folder, 2 Open) all jumped correctly. In the very first Total Commander
  run one jump (left panel) did not happen while the later ones did; not explained, not reproduced in 5 runs.
- Not verified: the user's actual dialog (Claude Code / Electron). It is expected to be the same standard dialog, but this
  was only checked against the equivalent dialog from a test program. The old tree-style `SHBrowseForFolder` dialog is a
  different dialog and is still not supported.

## Set Icon (added 2026-11-26, after release 0.4.0; not released yet)
New tray menu item **Set Icon** (menu is now: Run at Startup, Set Icon, separator, About, Exit). It opens one small native
dialog (`src/SetIconDialog.cpp`, template in `src/app.rc`): check box **Set custom icon**, button **Browse** with the chosen
file shown to its right, buttons **Apply** and **Cancel**. Requested by the product owner; recorded in `PRODUCT_SPEC.md`
(third change) and `CLAUDE.md`.
- Behavior: unticked = built-in icon, but the chosen path is kept and shown dimmed, so ticking again restores the choice
  without browsing. Browse and the path are only live while the box is ticked. Apply saves and changes the tray icon at
  once (`NIM_MODIFY`); Cancel changes nothing. Ticked with no file, or with a file that is not an icon: Apply explains and
  keeps the dialog open, nothing is saved. Only `.ico` files (no PNG).
- Storage: `HKCU\Software\TheLastFolder` with `CustomIconEnabled` (DWORD) and `CustomIconPath` (string), via the new
  `src/Settings.cpp`. No files. This is the first persistent setting; the earlier "no settings" rule was relaxed to that extent.
- At startup the saved icon is used before the tray icon is added; a missing or broken file silently falls back to the
  built-in icon and the saved choice is left alone. The exe's own icon in Explorer is not affected (it is part of the file).
- Verified (built exe, run from `src\out`, driven with messages to its own windows, registry checked after each step):
  first open (unticked, Browse and path dimmed, "(no file chosen)"); Cancel saves nothing; tick + Apply without a file is
  refused and nothing is saved; Browse (the real file chooser, driven by setting its file name) fills the path; Apply saves
  `enabled=1` and the path; untick + Apply saves `enabled=0` and **keeps the path**; reopening shows the unticked box with
  the path dimmed; ticking again needs no Browse; a text file named `.ico` is refused by Apply and nothing is saved.
  The tray icon itself was checked from the screen (position from `Shell_NotifyIconGetRect`, colour of the 16 px icon):
  violet built-in -> orange custom immediately after Apply -> violet after untick -> orange after tick; also orange at
  startup with the choice saved, and gone after a clean exit. The dialog was captured and looks as specified (ticked and unticked).
- Test-environment caveat: when the same exe was started and stopped many times in a row, Windows sometimes did not show the
  tray icon at all (`Shell_NotifyIconGetRect` returned "not visible" although the process was alive). The **release 0.4.0 exe
  behaved the same** in an alternating test, so this is Windows tray behavior and not caused by this feature. It made the
  "restart" and "missing file falls back" cases impossible to confirm visually in some runs; for the latter only "no crash,
  still running" is confirmed. Orange-at-startup was confirmed in an earlier clean run.
- Not verified: a real mouse click on the menu item and on the dialog's buttons (they were sent as messages); the dialog at
  display scaling above 100% (the template uses dialog units and per-monitor DPI awareness, but this was not looked at);
  `.ico` files with only large images (the closest image is scaled to the tray size); icons in `.exe`/`.dll` or PNG files
  (not supported).

## Run at Startup (added 2026-11-24, after release 0.4.0; not released yet)
The tray menu is now **Run at Startup** (checkable), a separator, **About**, **Exit**. Requested by the product owner; this
overrides the earlier "About and Exit only" and "automatic start excluded" rules (noted in `docs/PRODUCT_SPEC.md` and
`CLAUDE.md`). New module `src/Autostart.h/.cpp`; `MenuItem` got a `checked` flag and id 0 means a separator.
- Mechanism: one `REG_SZ` value `TheLastFolder` in `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`, holding the exe's
  full path in quotes. No admin rights, no service, no scheduled task, no file of our own.
- The check mark is read back from the registry every time the menu opens (single source of truth). It reads as OFF when
  the value points to a different exe (moved/renamed copy) and when Windows has switched the entry off itself (Task
  Manager / Settings write a "disabled" byte under `...\Explorer\StartupApproved\Run`). Turning it on clears that mark.
- Verified with a probe using a TEST value name (the real entry untouched): not registered, enable, disabled by Windows
  (reads OFF), re-enable clears the mark, entry for another exe (reads OFF), rewrite, disable removes everything; nothing
  left behind. Verified in the real app with messages to its own windows: the menu lists Run at Startup, separator,
  About, Exit in that order; clicking creates the value with the quoted path, clicking again removes it; the check mark
  is drawn when ON and not when OFF (read through UI Automation). The real startup entry was empty before and after.
- Not verified: an actual Windows login with the entry in place, and a real mouse click on the menu item (the click was
  sent as `WM_COMMAND`). Group policy that blocks writing the `Run` key is not handled specially: the toggle then simply
  does not take effect and the mark stays as it was.

## Application icon (added 2026-11-23)
The product owner's icon is embedded as a resource (`src/app.rc`, `src/resource.h`, compiled by `rc` in `build.bat`). It is
both the exe's icon in Explorer and the tray icon. The tray loads it at the system small-icon size (`SM_CXSMICON`): exactly
the hand-made 16x16 image at 100% scaling, the nearest image scaled at higher DPI. The hand-drawn placeholder folder was
removed from `TrayIcon.cpp`. The exe is about 170 KB.

**Large sizes (fixed after the first version looked small on the desktop).** The first `icon.ico` only held 16 and 32 px, so
Explorer had nothing larger to show when icons are enlarged. `assets/icon.ico` now holds **16, 32, 48, 64, 128 and 256 px**,
built by `assets/make-icon.ps1` from `assets/icon-source/`:
- 16 and 32 px: the hand-made images, unchanged.
- 64, 128, 256 px: the large artwork `06_violet_chrome.png` (a 64x76 pixel-art grid drawn at 4x), cropped to a 256x256 square
  (only empty rows removed, nothing scaled) and scaled with nearest neighbour, so the pixels stay sharp. 48 px is resampled
  (0.75 of the 64 px grid cannot be pixel-exact).
- Verified: loading the resource from the built exe at 16, 32, 48, 64, 96, 128 and 256 px returns exactly those sizes; the
  256 px image was viewed and shows the whole artwork, uncropped and sharp.
- Not verified: how it looks on a real desktop at each icon size, and the tray at scaling above 100% (scaled from the 16 or
  32 px image). A 24 px image would make 150% crisper if it looks soft. Windows caches icons: an existing shortcut may keep
  showing the old one until the icon cache refreshes (re-create the shortcut, or restart Explorer).

## Double Commander (added after v0.3.1)
Double Commander 1.2.9 has the same window class (`TTOTAL_CMD`) as Total Commander but a different structure: all
controls are plain `Window`s without text, and the path bar is a windowless graphic control, so nothing can be read from
the panels. UI Automation only exposes the tab captions (last folder name, not the path). The one usable source is the
main window title, which DC fills with the active panel's folder when its option **Show current directory in the main
window title bar** (config key `ShowCurDirTitleBar`, under Options > Miscellaneous) is on:
`<folder name> (<full path>\) - Double Commander <version>`.
- `FileManagers` tells DC from TC by the title (starts with / contains "Double Commander"), then parses the path from the
  parentheses that start with a drive letter (folder names may contain parentheses themselves).
- Verified against the **real** DC on this machine (read-only): the parsed folder matches the title, including a long path
  with spaces and a trailing backslash.
- Verified end to end with an imitation (`poc/faketc.exe ... dc`): dialog stays put when opened; after switching DC's panel
  and returning it jumps to the right folder, including `C:\Program Files (x86)`; Total Commander scenario still passes.
- **Needs the DC option turned on.** With it off the title has no path, so nothing happens (by design). The option was turned
  on by the user in their DC; this app never edits DC's configuration.
- **Confirmed by the product owner (2026-11-23): it works with the real Double Commander.** Exactly which navigations were
  tried (tab switch, panel switch, drive change) was not itemized, so those are not individually confirmed.

## Behavior (as specified by the product owner after testing v0.3.0)
1. A new dialog **opens as usual**, in the folder the program chose. Nothing jumps the first time a dialog is seen.
2. The user brings up Explorer or Total Commander. When the user **leaves** it, the folder it shows at that moment is
   remembered.
3. The user returns to the dialog. **Then** it jumps to the remembered folder. Only if a file manager was used since the
   dialog was last handled; coming back again without using a file manager does nothing.
(v0.3.0 jumped as soon as the dialog opened, using the folder of a file manager left in the background. That was wrong.)

The text below describes the mechanics; where it says "when a dialog becomes active ... jumps", read it with rules 1-3.

## Product
The Last Folder, `thelastfolder.exe`. Tray icon (About, Exit). No window, no hotkey, no settings.

## How it behaves (`src/`)
1. A global `EVENT_SYSTEM_FOREGROUND` hook sees every foreground change (own process skipped).
2. If the new foreground window is a file manager (Explorer `CabinetWClass`, Total Commander `TTOTAL_CMD`) it is remembered
   as "the last file manager". Total Commander's focus is followed with a hook limited to that process.
3. When a supported Open / Save As dialog of another program becomes the active window, 250 ms later the folder the last
   file manager is showing is read **live** and the dialog is navigated there. Nothing is typed by the user.
4. It jumps once per dialog per file-manager activation: coming back to a dialog does not jump again, unless a file manager
   was used in between. Not done when the dialog already shows that folder, or belongs to the file manager itself.
5. The jump is confirmed by a non-blocking timer (100 ms, up to 2 s). In Save As the user's file name is read first and
   put back afterwards. Failures are silent (no UI by design).
- **Never guesses:** unknown/unreadable folder, closed file manager, unknown active panel, non-local drive, special
  location ("This PC"), tree-style "Browse for Folder" dialogs: nothing happens.
- **Explorer:** documented COM route (`IShellWindows`, location URL of the matching window).
- **Total Commander:** read-only. Path text of the `TPathPanel` that belongs to the file list that last had focus
  (structure observed in the real TC 7.50 with `poc/wintree.exe`). Nothing is sent to TC; the clipboard is not used.

Modules: `main.cpp` (shell, timers), `TrayIcon`, `DialogWatcher` (WinEvents), `DialogAdapter` (detect/read/navigate),
`FileManagers` (Explorer + Total Commander). Removed: `QuickSwitchOverlay`, `FolderLookup`.

## Build
`cd src` then `build.bat` (x64 Release, `/MT`, `/W4 /WX`; toolchain `C:\BuildTools2022`, SDK 10.0.26100). Use
`build.bat out-test` if `out\thelastfolder.exe` is running (the linker cannot overwrite a running exe).
Result: 154,112 bytes, no errors or warnings. Links user32, shell32, gdi32, ole32, oleaut32, shlwapi, uuid.
Idle (one run, Windows 10): 0 ms CPU over 15 s, 8 MB working set, 1.5 MB private, 1 thread, 142 handles.

## Test results (Windows 10 x64, Swedish UI; test doubles in `poc/`, no desktop input synthesized)
`poc/dialoghost-gui.exe` = real Open/Save dialogs in another process (legacy + Common Item), Save As pre-filled with a name.
`poc/faketc.exe` = imitates Total Commander's window structure (classes and layout from the real TC 7.50).
| Scenario | Result |
|---|---|
| TC active panel unknown | no jump (correct) |
| TC right panel active -> CID Save As, legacy Save As | jumped to right panel's folder, file name kept |
| TC switched to left panel -> CID Open | jumped to left panel's folder |
| No file manager | no jump (correct) |
| Explorer window on `C:\Program Files`, `C:\Windows\System32`, `D:\` | folder read correctly |
| Explorer on "This PC" | empty, so no jump (correct) |
| Navigation PoC, 4 dialog types x 4 runs, path with a space | 16/16 verified |
Each scenario ran once or a few times; all in a harness that forces focus (`AttachThreadInput`), which is test-only.

## Findings worth knowing
- **`WM_SETTEXT` breaks Save As** when the dialog already has a file name (modal "save here?" box). Replaced by select-all +
  replace. This would have broken Notepad's Save As; it only surfaced because the test host got a pre-filled name.
- A background process normally cannot take the foreground; test dialogs sometimes lost focus to other apps within seconds.
  A real dialog the user is looking at keeps focus, so detection (which requires the dialog to be foreground) is fine there.
- Windows does not report a program's focused control while it is not the active window, which is why Total Commander's
  active panel must be tracked by hook rather than read on demand.

## NOT verified (please test)
- Real Total Commander 7.50 (`E:\Program\totalcmd`): focus tracking via `EVENT_OBJECT_FOCUS` on `TMyListBox`, and the panel
  mapping, were only checked against the imitation. Reading TC's path text with the real window was checked statically.
- Real Explorer foreground tracking (only the folder reading was tested; the foreground event itself was not obtainable in
  the test harness). Windows 11 Explorer tabs are not handled (first matching tab would be read).
- Real Notepad Save As and Open; Windows 11; English UI; multiple monitors; elevated dialogs; slow/network drives (rejected
  by design); Explorer or TC being unresponsive (COM call to Explorer could block the UI thread briefly).
- Whether the 250 ms delay is enough for slow dialogs. If a jump is missed, nothing retries it.

## Known limitations
- Only local drive-letter folders. No UNC/network drives, no virtual folders, no archives/FTP in TC.
- Silent failure by design: there is no way to see why a jump did not happen.
- Folder-picker dialogs are not supported. Control ids and class names are observed behavior, not documented contracts.
- Two instances are not prevented.

## Next task
User tries it: use Explorer or Total Commander, open Notepad's File > Open and Save As, report whether it jumps and to
which folder. Then fix what the real programs show, then Phase 5 (hardening) items.
