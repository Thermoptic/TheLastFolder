// Phase 1 PoC: detect a standard file dialog in another process, read its current folder,
// navigate it to a given path, and verify the change.
//   navpoc.exe info            -> print dialog + current folder
//   navpoc.exe go <folder>     -> navigate and verify (exit code 0 = verified)
// Only documented Win32 messaging is used: no injection, no hooks, no keystrokes.
#include <windows.h>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <io.h>
#include <fcntl.h>
#include <algorithm>

static constexpr int kIdFileName = 1148;  // filename combo (ComboBoxEx32 > ComboBox > Edit)
static constexpr int kIdOk = 1;           // Open / Save button

static std::wstring ClassOf(HWND h) { wchar_t c[128]; GetClassNameW(h, c, 128); return c; }

static std::wstring TextOf(HWND h) {
    wchar_t buf[2048]{};
    DWORD_PTR len = 0;
    if (!SendMessageTimeoutW(h, WM_GETTEXT, 2048, (LPARAM)buf, SMTO_ABORTIFHUNG, 500, &len)) return L"";
    return buf;
}

// Observed filename-edit control ids (Win10 22H2, Swedish UI, see STATUS.md):
//   1148 = Open (legacy + Common Item Dialog), 1001 = Save As, 1152 = folder picker.
static constexpr int kIdEditSave = 1001;
static constexpr int kIdEditFolderPicker = 1152;

struct EditCtx { HWND found; int id; };
static BOOL CALLBACK EnumEdit(HWND h, LPARAM lp) {
    auto* c = (EditCtx*)lp;
    int id = GetDlgCtrlID(h);
    if (ClassOf(h) == L"Edit" && (id == kIdFileName || id == kIdEditSave || id == kIdEditFolderPicker)) {
        c->found = h; c->id = id; return FALSE;
    }
    return TRUE;
}
static HWND FindFileNameEdit(HWND dlg, int* id = nullptr) {
    EditCtx c{nullptr, 0};
    EnumChildWindows(dlg, EnumEdit, (LPARAM)&c);
    if (id) *id = c.id;
    return c.found;
}

struct FindCtx { DWORD ownPid; HWND found; };
static BOOL CALLBACK EnumTop(HWND h, LPARAM lp) {
    auto* c = (FindCtx*)lp;
    if (!IsWindowVisible(h) || ClassOf(h) != L"#32770") return TRUE;
    DWORD pid = 0; GetWindowThreadProcessId(h, &pid);
    if (pid == c->ownPid) return TRUE;
    if (!GetDlgItem(h, kIdOk) || !FindFileNameEdit(h)) return TRUE;  // structural filter
    c->found = h;
    return FALSE;
}
static HWND FindDialog() {
    FindCtx c{GetCurrentProcessId(), nullptr};
    EnumWindows(EnumTop, (LPARAM)&c);
    return c.found;
}

struct ChildCtx { const wchar_t* cls; HWND found; };
static BOOL CALLBACK EnumChild(HWND h, LPARAM lp) {
    auto* c = (ChildCtx*)lp;
    if (ClassOf(h) == c->cls) { c->found = h; return FALSE; }
    return TRUE;
}
static HWND FindDesc(HWND root, const wchar_t* cls) {
    ChildCtx c{cls, nullptr};
    EnumChildWindows(root, EnumChild, (LPARAM)&c);
    return c.found;
}

// Current folder: the breadcrumb toolbar text under "Address Band Root" reads "<Label>: <path>".
// The label is localized, so only the text after the first ": " is used.
static std::wstring CurrentFolder(HWND dlg) {
    HWND band = FindDesc(dlg, L"Address Band Root");
    if (!band) return L"";
    HWND parent = FindDesc(band, L"Breadcrumb Parent");
    HWND tb = parent ? FindDesc(parent, L"ToolbarWindow32") : nullptr;
    if (!tb) return L"";
    std::wstring t = TextOf(tb);
    size_t p = t.find(L": ");
    return p == std::wstring::npos ? t : t.substr(p + 2);
}

static std::wstring Norm(std::wstring s) {
    while (s.size() > 3 && (s.back() == L'\\' || s.back() == L'/')) s.pop_back();
    std::transform(s.begin(), s.end(), s.begin(), ::towlower);
    return s;
}

static bool Navigate(HWND dlg, const std::wstring& path) {
    int editId = 0;
    HWND edit = FindFileNameEdit(dlg, &editId);
    HWND ok = GetDlgItem(dlg, kIdOk);
    // In a folder picker, "path + OK" would *select* the folder and close the dialog: not navigation.
    if (editId == kIdEditFolderPicker) return false;
    if (!edit || !ok) return false;
    DWORD_PTR r;
    wchar_t var[16]{};
    GetEnvironmentVariableW(L"NAVVAR", var, 16);
    const std::wstring v = var;
    wprintf(L"  [diag] variant=%s\n", v.empty() ? L"1" : v.c_str());
    if (v.empty() || v == L"2") {  // default: select all + replace selection, like typing over the text
        // (plain WM_SETTEXT, variant 1, fails in Save As dialogs that already hold a file name)
        SendMessageTimeoutW(edit, EM_SETSEL, 0, -1, SMTO_ABORTIFHUNG, 1000, &r);
        SendMessageTimeoutW(edit, EM_REPLACESEL, TRUE, (LPARAM)path.c_str(), SMTO_ABORTIFHUNG, 1000, &r);
    } else if (v == L"4") {  // set the text on the owning combo box instead of its edit
        HWND combo = GetParent(edit);
        SendMessageTimeoutW(combo, WM_SETTEXT, 0, (LPARAM)path.c_str(), SMTO_ABORTIFHUNG, 1000, &r);
    } else if (!SendMessageTimeoutW(edit, WM_SETTEXT, 0, (LPARAM)path.c_str(), SMTO_ABORTIFHUNG, 1000, &r)) {
        wprintf(L"  [diag] WM_SETTEXT failed, error=%lu\n", GetLastError());
        return false;
    }
    wprintf(L"  [diag] text now \"%s\" (edit id=%d)\n", TextOf(edit).c_str(), editId);
    if (v == L"3") {  // Enter key delivered to the edit control (message, not desktop input)
        PostMessageW(edit, WM_KEYDOWN, VK_RETURN, 0x001C0001);
        PostMessageW(edit, WM_KEYUP, VK_RETURN, 0xC01C0001);
        return true;
    }
    // Same as pressing the default button: a folder path in the filename box navigates, it does not select a file.
    const DWORD t0 = GetTickCount();
    const LRESULT res = SendMessageTimeoutW(dlg, WM_COMMAND, MAKEWPARAM(kIdOk, BN_CLICKED), (LPARAM)ok,
                                            SMTO_ABORTIFHUNG, 1000, &r);
    wprintf(L"  [diag] IDOK result=%lld error=%lu elapsed=%lums\n", (long long)res, GetLastError(), GetTickCount() - t0);
    for (int i = 0; i < 10 && !res; ++i) { Sleep(300); wprintf(L"  [diag] +%dms folder=\"%s\"\n", (i + 1) * 300, CurrentFolder(dlg).c_str()); }
    return res != 0;
}

// Diagnostic: list every visible #32770 of another process with all child windows (id + class).
static BOOL CALLBACK PrintChild(HWND h, LPARAM) {
    wprintf(L"    child id=%d class=%s text=\"%s\"\n", GetDlgCtrlID(h), ClassOf(h).c_str(), TextOf(h).substr(0, 60).c_str());
    return TRUE;
}
static BOOL CALLBACK EnumDiag(HWND h, LPARAM) {
    DWORD pid = 0; GetWindowThreadProcessId(h, &pid);
    if (!IsWindowVisible(h) || ClassOf(h) != L"#32770" || pid == GetCurrentProcessId()) return TRUE;
    wprintf(L"dialog 0x%p title=\"%s\"\n", h, TextOf(h).c_str());
    EnumChildWindows(h, PrintChild, 0);
    return TRUE;
}

int wmain(int argc, wchar_t** argv) {
    _setmode(_fileno(stdout), _O_U16TEXT);
    if (argc < 2) { wprintf(L"usage: navpoc info | children | go <folder>\n"); return 64; }
    if (std::wstring(argv[1]) == L"children") { EnumWindows(EnumDiag, 0); return 0; }
    HWND dlg = FindDialog();
    if (!dlg) { wprintf(L"FAIL: no supported dialog found\n"); return 1; }
    DWORD pid = 0; GetWindowThreadProcessId(dlg, &pid);
    wprintf(L"dialog hwnd=0x%p pid=%lu title=\"%s\"\n", dlg, pid, TextOf(dlg).c_str());
    std::wstring before = CurrentFolder(dlg);
    wprintf(L"current folder: \"%s\"\n", before.c_str());
    if (std::wstring(argv[1]) == L"info") return 0;

    std::wstring target = argc > 2 ? argv[2] : L"";
    if (!Navigate(dlg, target)) { wprintf(L"FAIL: not navigated (folder picker unsupported by this method, or elevated/hung host)\n"); return 2; }
    // PoC verification: bounded wait (the product will use WinEvents instead of polling).
    for (int i = 0; i < 20; ++i) {
        Sleep(100);
        if (!IsWindow(dlg)) { wprintf(L"FAIL: dialog closed (target was treated as a file?)\n"); return 3; }
        std::wstring now = CurrentFolder(dlg);
        if (Norm(now) == Norm(target)) { wprintf(L"OK: folder is now \"%s\"\n", now.c_str()); return 0; }
    }
    wprintf(L"FAIL: folder is \"%s\", expected \"%s\"\n", CurrentFolder(dlg).c_str(), target.c_str());
    return 4;
}
