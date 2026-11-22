// Read-only probe: lists top-level windows of a process and all their descendants (class, id, text, visible).
// Usage: wintree.exe <pid>
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <io.h>
#include <fcntl.h>
#include <string>

static DWORD g_pid;

static std::wstring ClassOf(HWND h) { wchar_t c[128]{}; GetClassNameW(h, c, 128); return c; }
static std::wstring TextOf(HWND h) {
    wchar_t b[512]{};
    DWORD_PTR n = 0;
    SendMessageTimeoutW(h, WM_GETTEXT, 512, reinterpret_cast<LPARAM>(b), SMTO_ABORTIFHUNG, 300, &n);
    return b;
}
static void Print(HWND h, int depth) {
    RECT r{};
    GetWindowRect(h, &r);
    std::wstring t = TextOf(h);
    for (auto& ch : t) if (ch < 32) ch = L' ';
    if (depth == 0) wprintf(L"hwnd=0x%p ", h);
    wprintf(L"%*s%s id=%d vis=%d rect=%ldx%ld text=\"%s\"\n", depth * 2, L"", ClassOf(h).c_str(), GetDlgCtrlID(h),
            IsWindowVisible(h) ? 1 : 0, r.right - r.left, r.bottom - r.top, t.substr(0, 120).c_str());
}
static BOOL CALLBACK Child(HWND h, LPARAM) { Print(h, 1); return TRUE; }
static BOOL CALLBACK Top(HWND h, LPARAM) {
    DWORD pid = 0;
    GetWindowThreadProcessId(h, &pid);
    if (pid != g_pid) return TRUE;
    Print(h, 0);
    EnumChildWindows(h, Child, 0);
    GUITHREADINFO gti{sizeof(gti)};
    if (GetGUIThreadInfo(GetWindowThreadProcessId(h, nullptr), &gti)) {
        wprintf(L"FOCUS thread-of-top-level: focus=0x%p (%s) active=0x%p fg=0x%p\n", gti.hwndFocus,
                gti.hwndFocus ? ClassOf(gti.hwndFocus).c_str() : L"-", gti.hwndActive, GetForegroundWindow());
        if (gti.hwndFocus) { RECT r{}; GetWindowRect(gti.hwndFocus, &r); wprintf(L"FOCUS rect: left=%ld right=%ld\n", r.left, r.right); }
    }
    return TRUE;
}

int wmain(int argc, wchar_t** argv) {
    _setmode(_fileno(stdout), _O_U16TEXT);
    if (argc < 2) return 64;
    g_pid = wcstoul(argv[1], nullptr, 10);
    EnumWindows(Top, 0);
    return 0;
}
