// Test double for Total Commander's window structure (observed in a real TC 7.50 with poc/wintree.exe):
//   top-level class TTOTAL_CMD; per panel a TPathPanel (zero height, text "d:\dir\*.*") and a TMyListBox.
// Usage: faketc.exe <leftFolder> <rightFolder> <left|right|none> [dc]   (initial focused file list)
// Posting WM_USER with wParam 0 / 1 moves focus to the left / right list. Posting WM_USER+1 brings it to the front.
// With the 4th argument "dc" it imitates Double Commander instead (see DcTitle below).
#include <windows.h>
#include <string>

static HWND g_list[2];
static int g_active = -1;  // file list that should have focus; like the real TC, restore it on activation

// Test harness only: a background process normally cannot take the foreground. Attaching to the current
// foreground thread's input queue for the call is the documented workaround.
static void Raise(HWND top) {
    const DWORD fgThread = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
    const DWORD me = GetCurrentThreadId();
    AttachThreadInput(me, fgThread, TRUE);
    BringWindowToTop(top);
    SetForegroundWindow(top);
    AttachThreadInput(me, fgThread, FALSE);
}

// Double Commander mode ("dc" as 4th argument): same window class, no readable panels, and the active panel's folder
// only in the title, formatted like the real thing: "<folder name> (<full path>\) - Double Commander 1.2.9~395".
static std::wstring g_dcFolders[2];
static bool g_dcMode = false;
static std::wstring DcTitle(int side) {
    std::wstring p = g_dcFolders[side];
    if (p.empty() || p.back() != L'\\') p += L'\\';
    std::wstring leaf = g_dcFolders[side];
    const size_t slash = leaf.find_last_of(L'\\');
    if (slash != std::wstring::npos) leaf = leaf.substr(slash + 1);
    return leaf + L" (" + p + L") - Double Commander 1.2.9~395";
}

static LRESULT CALLBACK Proc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_USER + 1) { Raise(h); return 0; }  // "the user brings the file manager to the front"
    if (m == WM_USER && w < 2 && g_dcMode) { SetWindowTextW(h, DcTitle(static_cast<int>(w)).c_str()); return 0; }
    if (m == WM_USER && w < 2) { g_active = static_cast<int>(w); SetFocus(g_list[w]); return 0; }
    if (m == WM_SETFOCUS && g_active >= 0) { SetFocus(g_list[g_active]); return 0; }
    if (m == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(h, m, w, l);
}

static void RegisterChildClass(HINSTANCE inst, const wchar_t* name) {
    WNDCLASSW wc{};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = inst;
    wc.lpszClassName = name;
    RegisterClassW(&wc);
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR cmd, int) {
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argc < 4) return 64;
    const std::wstring paths[2] = {std::wstring(argv[1]) + L"\\*.*", std::wstring(argv[2]) + L"\\*.*"};
    const std::wstring active = argv[3];
    (void)cmd;

    WNDCLASSW wc{};
    wc.lpfnWndProc = Proc;
    wc.hInstance = inst;
    wc.lpszClassName = L"TTOTAL_CMD";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassW(&wc);
    RegisterChildClass(inst, L"TPathPanel");
    RegisterChildClass(inst, L"TMyListBox");

    g_dcMode = argc >= 5 && std::wstring(argv[4]) == L"dc";
    g_dcFolders[0] = argv[1];
    g_dcFolders[1] = argv[2];
    const std::wstring title = g_dcMode ? DcTitle(active == L"left" ? 0 : 1) : L"Fake Total Commander";
    HWND top = CreateWindowExW(0, L"TTOTAL_CMD", title.c_str(), WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 1020, 500,
                               nullptr, nullptr, inst, nullptr);
    if (!top) return 1;
    for (int i = 0; i < 2 && !g_dcMode; ++i) {
        const int x = i * 510;
        CreateWindowExW(0, L"TPathPanel", paths[i].c_str(), WS_CHILD | WS_VISIBLE, x, 0, 500, 0, top, nullptr, inst, nullptr);
        g_list[i] = CreateWindowExW(0, L"TMyListBox", L"", WS_CHILD | WS_VISIBLE, x, 20, 500, 380, top, nullptr, inst, nullptr);
    }
    Raise(top);
    if (!g_dcMode && active == L"left") { g_active = 0; SetFocus(g_list[0]); }
    else if (!g_dcMode && active == L"right") { g_active = 1; SetFocus(g_list[1]); }

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0) > 0) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return 0;
}
