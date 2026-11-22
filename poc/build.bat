@echo off
rem Builds the PoC tools (x64 Release, /MT). Run from the poc directory.
call "C:\BuildTools2022\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
if not exist out mkdir out
for %%F in (dialoghost uiadump navpoc) do (
  if exist %%F.cpp (
    cl /nologo /std:c++20 /O2 /MT /EHsc /W4 /DUNICODE /D_UNICODE %%F.cpp /Fo:out\ /Fe:out\%%F.exe ^
       user32.lib ole32.lib oleaut32.lib comdlg32.lib shell32.lib uuid.lib || exit /b 1
  )
)
