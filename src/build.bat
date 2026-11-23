@echo off
rem Builds thelastfolder.exe (x64 Release, /MT). Run from the src directory.
rem Optional argument: output directory (default "out"). Useful when out\thelastfolder.exe is running.
set OUTDIR=%~1
if "%OUTDIR%"=="" set OUTDIR=out
call "C:\BuildTools2022\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1 || exit /b 1
if not exist %OUTDIR% mkdir %OUTDIR%
rem Resources: the application icon (assets\icon.ico) is embedded in the exe.
rc /nologo /fo %OUTDIR%\app.res app.rc || exit /b 1
cl /nologo /std:c++20 /O2 /MT /EHsc /W4 /WX /DUNICODE /D_UNICODE /DWIN32_LEAN_AND_MEAN /DNOMINMAX ^
   main.cpp TrayIcon.cpp DialogAdapter.cpp DialogWatcher.cpp FileManagers.cpp ^
   /Fo:%OUTDIR%\ /Fe:%OUTDIR%\thelastfolder.exe ^
   /link /SUBSYSTEM:WINDOWS /MANIFEST:EMBED user32.lib shell32.lib gdi32.lib ole32.lib oleaut32.lib shlwapi.lib uuid.lib %OUTDIR%\app.res || exit /b 1
