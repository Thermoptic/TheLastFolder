# Phase 0 Design Note

Findings below were observed on Windows 10 22H2 x64, Swedish UI, with real dialogs created by a separate test process (`poc/dialoghost.exe`). Not yet tested: Windows 11, English UI, other host apps.

## 1. Dialog families
| Family | Result |
|---|---|
| Legacy `GetOpenFileName` | Navigation verified |
| Legacy `GetSaveFileName` | Navigation verified |
| Common Item Dialog Open | Navigation verified |
| Common Item Dialog Save | Navigation verified |
| Folder picker (`FOS_PICKFOLDERS`, "Select Folder") | Navigation verified (earlier marked "not navigable": that was an untested assumption, corrected 2026-11-28) |
| Custom dialogs (Electron, Qt, Office, Java) | Not investigated; out of scope for first release |

## 2. Detection (planned, product phase)
`SetWinEventHook` out-of-context: `EVENT_OBJECT_SHOW` (+ `EVENT_SYSTEM_FOREGROUND`) to find candidates, `EVENT_OBJECT_DESTROY` / `EVENT_OBJECT_LOCATIONCHANGE` to follow them. The PoC does a one-shot `EnumWindows` instead of hooks.

## 3. Filtering
Top-level, visible, class `#32770`, other process, **and** a button with id 1 **and** an `Edit` child with id 1148 (Open), 1001 (Save As) or 1152 (folder picker). Class `#32770` alone is not enough: it is the generic dialog class.
Window/control **names are localized** (Swedish here) and must never be used for matching; only classes and numeric ids are.

## 4. UI Automation
UIA exposes the tree (`DUIViewWndClassName` > `HWNDView`, `ComboBoxEx32`/`Edit` aid 1148, `Button` aid 1, `Address Band Root` > `Breadcrumb Parent` > `ToolbarWindow32`), but **navigation did not need UIA**; plain Win32 messaging was enough. UIA remains a fallback only if Win32 reading fails.

## 5. Navigation method (works)
1. Replace the filename `Edit` text with the folder path: `EM_SETSEL(0, -1)` then `EM_REPLACESEL`.
2. `WM_COMMAND(IDOK, BN_CLICKED)` to the dialog (same as pressing the default button).
A folder path in the filename box navigates instead of choosing a file. No keystrokes, injection, or hooks into the host.
Both calls use `SendMessageTimeout` so a hung host cannot hang this process.

**Do not use plain `WM_SETTEXT` for step 1.** It worked in the first PoC only because the test dialogs had an empty
filename box. In a Save As dialog that already holds a name (Notepad pre-fills one) the dialog ignores the new text on
OK and instead asks "save Untitled.txt here / in Documents instead?" (a modal Yes/No box that also blocks our
`SendMessage`). Measured on legacy and Common Item Save As: `WM_SETTEXT` failed 6/6, select-all + replace worked 16/16
across all four Open/Save dialog types, including a path with a space. Other variants tried and rejected: Enter key
message posted to the edit, and `WM_SETTEXT` on the owning combo box (both left the folder unchanged).
The navigation overwrites the filename box, so the product reads it first and restores it afterwards.

## 6. Verification
Read the current folder from the `ToolbarWindow32` under `Breadcrumb Parent` inside `Address Band Root`. Its text is `<localized label>: <path>`; only the part after the first `": "` is used. Compare case-insensitively with the target. PoC polls up to 2 s; the product should wait on a WinEvent (name change) rather than poll.

## 7. Limitations
- **Folder picker ("Select Folder")**: supported with the same method as Open and Save As. This note originally said that path + OK would
  select the folder and close the dialog, and the picker was refused on that basis. It was never tested. Measured on 2026-11-28
  against a real `IFileOpenDialog` with `FOS_PICKFOLDERS` in another process (`poc/dialoghost-gui.exe cid-folder`): replacing the
  filename box text with a folder path and pressing OK moves the dialog to that folder, leaves it open, and empties the filename
  box; pressing "Select Folder" afterwards returns exactly that folder (checked for `C:\Users` and `C:\Program Files (x86)`).
  The old tree-style "Browse for Folder" (`SHBrowseForFolder`) is a different dialog and is still not recognised.
- **Non-existent folder**: the dialog shows a modal error box that blocks the message; `SendMessageTimeout` returns after 1 s. Product must check the path exists (and is accessible) *before* navigating.
- **Elevated host**: UIPI blocks window messages from a lower-integrity process. Not yet tested; expected to fail safely (timeout / no-op). No bypass will be attempted.
- **Filename box content** is overwritten by the path; any text the user typed there is lost.
- Control ids are observed behavior, not a documented contract; re-verify on Windows 11.
