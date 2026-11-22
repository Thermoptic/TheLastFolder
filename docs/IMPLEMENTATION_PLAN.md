# Implementation Plan

Build in small verified stages. Do not skip the proof of concept.

## Phase 0 — Technical investigation

Before implementing the product, report:

1. Which Windows file-dialog families will be targeted first?
2. Which WinEvent events are useful for detecting candidate dialogs?
3. How will unrelated windows be filtered out?
4. What UI Automation controls/patterns are exposed by the test dialogs?
5. How can the existing dialog be navigated to a new folder?
6. How will navigation success be verified?
7. What limitations exist for elevated processes and custom dialogs?

Create a short technical design note. Do not over-engineer.

## Phase 1 — Navigation proof of concept

Build the smallest possible x64 executable or prototype that:

1. Detects one supported native Open dialog in another process.
2. Identifies the dialog and its current folder.
3. Displays a basic temporary window or test trigger.
4. Accepts a known folder path.
5. Navigates the existing dialog to that path.
6. Verifies the current folder actually changed.

Test first with Windows Notepad or another readily available standard Windows application. Also test Save As.

Do not proceed until the core navigation is demonstrated.

Deliver:
- Source code.
- Build instructions.
- Exact test applications and dialog types tested.
- Observed limitations.

## Phase 2 — Minimal application shell

Add:
- Background message loop.
- System tray icon.
- Right-click menu: About, Exit.
- Correct cleanup of hooks, windows, COM objects, and tray icon.

No settings or configuration files.

> **Phases 3 and 4 below are superseded** by the scope change in `docs/PRODUCT_SPEC.md`: no overlay, no search, no
> hotkey. The replacement ("automatic jump to the last file manager's folder") is documented in `STATUS.md`.
> An overlay with Ctrl+G was built and tested as Phase 3 and then removed at the product owner's request.

## Phase 3 — Quick Switch overlay (superseded)

Add:
- Compact search input.
- Small result list.
- Keyboard navigation.
- Escape handling.
- Positioning near the associated dialog.
- Correct focus handling and cleanup.

Use a fixed activation method for the initial release. Prefer `Ctrl+G` only after checking conflicts and confirming that it behaves correctly.

## Phase 4 — On-demand folder lookup

Add:
- Known user folder roots.
- Folder-name partial search.
- Case-insensitive matching.
- Exact/prefix match ranking.
- Full-path input.
- Maximum of approximately 8 visible results.
- Safe handling of inaccessible or disconnected paths.

No permanent indexing, database, or background scanner.

## Phase 5 — Compatibility and hardening

Test:
- Open dialog.
- Save As dialog.
- Folder selection dialog where feasible.
- Dialog movement and closing.
- Multiple dialogs sequentially.
- Multiple monitors.
- 100%, 125%, 150%, and 200% display scaling.
- Windows 10 and Windows 11.
- Non-existent folders.
- Network/removable drives.
- Elevated host application behavior.

Unsupported cases must fail safely and be documented.

## Phase 6 — Release

Produce:
- `thelastfolder.exe`
- Full source code.
- Visual Studio solution/project.
- README with build instructions.
- `STATUS.md` with tested compatibility and known limitations.

Build x64 Release. Measure idle CPU, idle memory, and executable size. Verify the binary on a clean Windows environment if possible.

## Definition of done

The first release is complete when:
- The process runs quietly in the background.
- Its tray icon reliably indicates that it is running.
- Right-click exposes About and Exit.
- A supported file dialog can be detected.
- The temporary Quick Switch overlay can be invoked.
- Folder lookup returns useful results.
- Selecting a result navigates the existing dialog.
- Escape and dialog closure clean up correctly.
- The app remains lightweight while idle.
- No unrelated features have been added.
