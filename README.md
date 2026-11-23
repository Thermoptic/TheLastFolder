<p align="center">
  <img src="assets/icon-source/06_violet_chrome.png" alt="The Last Folder" width="256" height="304">
</p>

# The Last Folder

**The Open and Save As dialog finally knows where you were headed.**

You know the routine. You are deep in a folder in Explorer, Total Commander or Double Commander, you alt-tab to some other program, hit
*File > Open*, and the dialog cheerfully offers you whatever directory that program last touched in 2019. Then comes the
click-through: drive, folder, folder, folder, pause to question your life choices, folder.

The Last Folder is a small background utility that closes that loop. Use a file manager, return to the dialog, and the
dialog is already sitting in the folder you just left. No hotkey, no overlay, no window. Take the path of least
resistance, quite literally.

## How it feels in use

1. The dialog opens as it always has, in the folder the program normally picks.
2. You switch to Explorer, Total Commander or Double Commander and go where you want to be.
3. You switch back to the dialog. It jumps to that folder.
4. You open or save the file like you would have anyway.

It only acts if you have actually used a file manager since the dialog appeared. If you have not, the dialog is left
alone. It does not second-guess you, and it does not redecorate while you are still thinking.

## Supported file managers

| File manager | Works out of the box | What it reads |
|---|---|---|
| **Windows Explorer** | Yes | The folder of the window you were in, via the Shell's documented window list |
| **Total Commander** | Yes | The path bar of the panel you used last (tested with Total Commander 7.50) |
| **Double Commander** | **Only with one setting turned on** | The folder shown in its window title |

**Double Commander needs a setting.** It works only if you open *Configuration > Options > Miscellaneous* and tick
**Show current directory in the main window title bar**. Double Commander draws its path bar itself, so the title bar is
the only place the folder can be read from, and by default it is not there. Without the box ticked the program does
nothing for Double Commander (it will not guess), which is the safe kind of useless.

Whichever you use, the same rules apply: it reads, it never writes, and it only sees local drive-letter folders. Notes on
what is untested are under [Status](#status).

## Small on purpose

There is no UI to speak of. The tray icon is the whole interface, and the whole menu is **About** and **Exit**. A
utility that needs a settings page to explain what it does has already lost the argument.

- **One portable exe, roughly 150 KB.** No installer, no runtime to hunt down, nothing beyond the Windows system
  libraries. You can carry it on a USB stick, in your pocket, or in a small bird's beak.
- **Idle means idle.** It is event-driven, so it does nothing until Windows tells it something happened. One measurement on
  Windows 10 showed 0 ms of CPU over 15 seconds and about 8 MB of memory, on a single thread.
- **Nothing to configure, and nothing to leak.** No settings, config files, database, indexer, logging, telemetry or network
  access. It cannot phone home because it has never learned the number.
- **Native.** C++20 and plain Win32, built with MSVC. No framework was harmed.

## Under the hood

- **Tracking your file manager.** A WinEvent hook reports every change of the foreground window. When a supported file
  manager comes to the front it is noted, and when you leave it, the folder it was showing is remembered. Explorer is
  read through the Shell's documented window list (`IShellWindows`). For Total Commander it reads the path bar of the
  panel that last had focus. Double Commander draws its path bar itself, so nothing there can be read; it is read from
  the window title instead (see [Supported file managers](#supported-file-managers)). Double Commander wears the same
  window class as Total Commander, so the title is also how the two are told apart. All of it is strictly read-only:
  nothing is sent to any of these programs, and your clipboard is left alone.
- **Recognising a dialog.** A standard Open or Save As dialog is identified by structure (the dialog window class, a
  filename box with a known control id, and a default button), never by title. That keeps it language-independent.
- **The jump.** The existing dialog is navigated, not replaced. The folder path goes into the filename box and the
  default button is pressed, which is the dialog's own way of being told to go somewhere. The box is filled by selecting
  all and replacing the text rather than with a plain `WM_SETTEXT`, because a Save As dialog that already holds a name
  ignores the latter and offers to save the old file instead. The existing file name is restored afterwards, and the
  move is verified before it counts.
- **Polite by construction.** No code injection, no patching, no hooks inside other processes. If the folder is
  unknown, unreadable or not a local drive-letter path, it does nothing. It would rather stay put than guess wrong.

## Status

A working prototype, version 0.3. Tried by hand and automatically tested against test dialogs, on Windows 10 x64 only.
Expect rough edges, and treat anything not listed below as unverified rather than supported.

Known limits:

- Local drive-letter folders only. Network paths, virtual locations such as *This PC*, and archives are skipped.
- Folder-selection dialogs ("Browse for folder") are not supported. Their *OK* means *pick this*, not *go there*.
- Double Commander only works with the title-bar setting above, and its title can only show the active panel's folder.
- Windows 11 is untested, and Explorer tabs are not handled.
- Dialogs of programs running elevated are untested.
- Dialogs a program draws itself rather than leaving to Windows (some Electron, Qt and Java applications) are not
  recognised.

[`STATUS.md`](STATUS.md) has the exact test matrix, including what was not tested.

## Build

You need the Visual Studio Build Tools (MSVC) and a Windows 10 or 11 SDK. From the `src` folder:

```
build.bat
```

That produces `src\out\thelastfolder.exe` (x64, Release, static runtime). The script expects the Build Tools in
`C:\BuildTools2022`, so adjust the path at the top if yours live elsewhere. If the program is running, exit it from the
tray first, or build elsewhere with `build.bat some-other-folder`. Windows will not let a linker overwrite an exe that is
in use, which is the one thing it is consistent about.

## Repository layout

- `src/` the application
- `assets/` the application icon (`icon.ico`, 16 to 256 px, embedded in the exe). `make-icon.ps1`
  rebuilds it from the images in `icon-source/`
- `poc/` the navigation proof of concept and the test tools: a test dialog host, an imitation Total Commander and a few
  window probes
- `docs/` product specification, technical notes, design note and implementation plan
- `STATUS.md` current behavior, test results and known limitations
- `CLAUDE.md` working rules used during AI-assisted development
