# The Last Folder

A tiny Windows background utility that recreates the essential **Quick Switch** workflow from Listary.

**Application name:** The Last Folder  
**Executable:** `thelastfolder.exe`  
**Platform:** Windows 10/11, x64  
**Language:** C++20, MSVC, Win32 API

## What it does

The Last Folder remembers the folder of the file manager you used last (Windows Explorer or Total Commander). When a standard Open / Save As dialog of another program then becomes active, the *existing dialog* jumps to that folder automatically. There is nothing to type and no window to operate. (This replaced the original overlay-with-search design; see the scope-change note in `docs/PRODUCT_SPEC.md`.)

The application runs quietly in the background and shows a system tray icon while active.

## Non-goals

This is not a Listary replacement. Do not add global file search, application launching, file previews, file management, indexing services, settings panels, or unrelated productivity features.

## Start here

Give Claude Code the repository and ask it to read, in order:

1. `CLAUDE.md`
2. `docs/PRODUCT_SPEC.md`
3. `docs/TECHNICAL_NOTES.md`
4. `docs/IMPLEMENTATION_PLAN.md`

Claude must begin with the technical investigation and proof of concept. Do not implement the full product before proving that it can reliably navigate a real dialog created by another process.
