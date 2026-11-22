# The Last Folder

**Open and Save As dialogs that remember where you were.**

The Last Folder is a tiny background utility for Windows. When you have just been working in a folder in
**Windows Explorer** or **Total Commander** and then go back to an Open or Save As dialog, the dialog jumps to that
same folder. No typing, no hotkey, no window.

## What it is good for

You are in a folder in your file manager, switch to another program, and choose *File > Open* or *Save As*. The dialog
opens wherever that program last happened to be, so you spend the next few seconds clicking your way back to the folder
you were just in.

With The Last Folder you do this instead:

1. The dialog opens as usual, in the folder the program normally uses.
2. Switch to Explorer or Total Commander and go to the folder you want.
3. Switch back to the dialog. It jumps to that folder.
4. Open or save your file as normal.

It only acts when you have actually used a file manager since the dialog opened. If you do not, the dialog is left
alone.

## Small by design

- **No user interface.** The only thing you see is a tray icon. Right-click it for **About** and **Exit**. There is no
  main window, no overlay, no search box and no settings.
- **Tiny.** One portable `thelastfolder.exe` of about 150 KB. No installer, no runtime to install, no dependencies
  beyond the Windows system libraries.
- **Light on resources.** It is event-driven and does nothing while idle. Measured once on Windows 10: 0 ms of CPU over
  15 seconds and roughly 8 MB of memory, with a single thread.
- **No configuration.** No settings, no config files, no database, no indexing, no logging, no telemetry and no network
  access.
- **Native.** C++20 and the plain Win32 API, built with MSVC. No frameworks.

## How it works

- **Following your file manager.** A Windows event hook reports which window you bring to the front. When that window
  is Explorer or Total Commander, The Last Folder notes it, and when you leave it, it remembers the folder it was
  showing. Explorer is read through the Windows Shell's documented window list. Total Commander is read from the path
  bar of the panel you last used. Nothing is ever sent to either program, and the clipboard is not touched.
- **Recognising a dialog.** A standard Open or Save As dialog of another program is recognised by its structure (the
  dialog window class plus its filename box and default button), not by its title, so it works in any language.
- **Jumping.** The existing dialog is navigated, not replaced. The folder path is put into the dialog's filename box and
  the default button is pressed, which is how a dialog is told to go to a folder. In Save As the file name that was
  already there is put back afterwards, and the move is checked before it counts as done.
- **Safe by default.** No code is injected into other programs and nothing is patched or hooked inside them. If the
  folder is unknown or cannot be read, nothing happens: it never guesses.

## Current status

A working prototype (version 0.3). It has been tried by hand and works, and it has been tested automatically against
test dialogs. Tested on Windows 10 x64 only.

Known limits:

- Local drive-letter folders only. Network paths, virtual folders such as *This PC*, and archives are skipped.
- Folder-selection dialogs ("Browse for folder") are not supported.
- Windows 11 is untested, and Explorer tabs there are not handled.
- Dialogs of programs running as administrator are untested.
- Dialogs drawn by the program itself rather than by Windows (some Electron, Qt or Java applications) are not
  recognised.

See [`STATUS.md`](STATUS.md) for exactly what has been tested and what has not.

## Build

Requires Visual Studio Build Tools (MSVC) with a Windows 10/11 SDK. From the `src` folder:

```
build.bat
```

This produces `src\out\thelastfolder.exe` (x64, Release, static runtime). `build.bat` expects the Build Tools in
`C:\BuildTools2022`; edit the path at the top if yours are elsewhere. If `thelastfolder.exe` is running, exit it from the
tray first, or build elsewhere with `build.bat some-other-folder`.

## Repository layout

- `src/` the application
- `poc/` the navigation proof of concept and the test tools: a test dialog host, an imitation Total Commander, and
  window probes
- `docs/` product specification, technical notes, design note and implementation plan
- `STATUS.md` current behavior, test results and known limitations
- `CLAUDE.md` working rules used during AI-assisted development
