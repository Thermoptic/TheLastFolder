# Instructions for Claude Code

You are implementing **The Last Folder**, a minimal native Windows utility.

Before writing code, read all documents in `docs/`.

## Mandatory working rules

1. Start with Phase 0 in `docs/IMPLEMENTATION_PLAN.md`.
2. Investigate Windows dialog detection and navigation before designing the rest of the application.
3. Build a small proof of concept and test it against real dialogs before adding folder search, history, polish, or optional features.
4. Do not claim a dialog type or host application is supported until it has been tested.
5. Keep the implementation small, readable, modular, and native.
6. Prefer documented Windows APIs. Avoid DLL injection, process patching, undocumented hooks, and invasive techniques.
7. Do not add dependencies unless they solve a demonstrated problem.
8. Do not create a settings UI, configuration system, database, background indexer, installer, updater, telemetry, or logging service.
9. Do not silently change the product scope.
10. Maintain a `STATUS.md` with completed work, current limitations, build/test results, and the next concrete task.

## Product identity

- Display name: The Last Folder
- Executable filename: `thelastfolder.exe`
- Internal project name: `TheLastFolder`

## Required baseline

- Windows 10 and Windows 11, x64.
- C++20 and MSVC.
- Native Win32 implementation.
- One portable executable.
- A system tray icon while running.
- Right-click tray menu with **About** and **Exit** only.
- No conventional main window.
- Temporary Quick Switch overlay only when relevant.
- No settings or configuration files.

## Build discipline

Use a clean x64 Release build. Prefer `/MT` if compatible with the selected toolchain and dependencies, but do not sacrifice correctness merely to force static runtime linking. Report the actual output size and dependencies; do not promise a specific EXE size before measuring.

At each phase:
- Build.
- Run relevant tests.
- Record observed results.
- Document unsupported cases.
- Stop and fix foundational failures before proceeding.

## First response / first task

Do not immediately generate the whole application.

First explain:
- How you will detect standard Windows file dialogs in other processes.
- How you will distinguish relevant dialogs from unrelated windows.
- How you will identify the current folder and navigate the existing dialog.
- Which dialog families are realistically supportable.
- What limitations arise from integrity levels, UI Automation access, and custom application dialogs.

Then implement only the smallest proof of concept described in `docs/IMPLEMENTATION_PLAN.md`.
