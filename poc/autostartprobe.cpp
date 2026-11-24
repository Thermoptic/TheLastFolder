// Probe for src/Autostart.cpp. Uses a TEST value name so the real startup entry is never touched.
// Usage: autostartprobe.exe status | enable | disable
#include <windows.h>
#include <cstdio>
#include <cwchar>
#include "../src/Autostart.h"

static const wchar_t kTestName[] = L"TheLastFolderTEST";

int wmain(int argc, wchar_t** argv) {
    if (argc < 2) return 64;
    if (wcscmp(argv[1], L"enable") == 0) wprintf(L"SetEnabled(true)  -> %d\n", Autostart::SetEnabled(true, kTestName) ? 1 : 0);
    else if (wcscmp(argv[1], L"disable") == 0) wprintf(L"SetEnabled(false) -> %d\n", Autostart::SetEnabled(false, kTestName) ? 1 : 0);
    wprintf(L"IsEnabled         -> %d\n", Autostart::IsEnabled(kTestName) ? 1 : 0);
    return 0;
}
