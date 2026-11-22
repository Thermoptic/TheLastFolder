// Probe: uses the product's FileManagers code on a given window and prints the folder it resolves.
// Usage: fmprobe.exe <hwnd-decimal>
#include <windows.h>
#include <objbase.h>
#include <cstdio>
#include <cstdlib>
#include <io.h>
#include <fcntl.h>
#include "../src/FileManagers.h"

int wmain(int argc, wchar_t** argv) {
    if (argc < 2) return 64;
    _setmode(_fileno(stdout), _O_U16TEXT);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    HWND h = reinterpret_cast<HWND>(static_cast<UINT_PTR>(_wtoi64(argv[1])));
    FileManagers fm;
    fm.NoteForeground(h);                  // the file manager comes to the front ...
    fm.NoteForeground(GetDesktopWindow()); // ... and the user leaves it: its folder is remembered
    wprintf(L"serial=%u pid=%lu remembered=\"%s\"\n", fm.Serial(), fm.LastPid(), fm.Remembered().c_str());
    fm.Stop();
    CoUninitialize();
    return 0;
}
