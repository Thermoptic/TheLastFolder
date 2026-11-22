#include "FileManagers.h"
#include <ole2.h>  // COM base types; not pulled in by WIN32_LEAN_AND_MEAN
#include <exdisp.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <algorithm>
#include <cwctype>
#include <vector>

FileManagers* FileManagers::self_ = nullptr;

namespace {

std::wstring ClassOf(HWND h) {
    wchar_t c[128]{};
    GetClassNameW(h, c, 128);
    return c;
}

std::wstring TextOf(HWND h) {
    wchar_t b[2048]{};
    DWORD_PTR n = 0;
    if (!SendMessageTimeoutW(h, WM_GETTEXT, 2048, reinterpret_cast<LPARAM>(b), SMTO_ABORTIFHUNG, 300, &n)) return L"";
    return b;
}

// Only existing folders on local drive letters are accepted. UNC paths and network drives are rejected
// on purpose: probing a disconnected share can block this thread.
bool IsLocalDirectory(const std::wstring& p) {
    if (p.size() < 3 || !std::iswalpha(p[0]) || p[1] != L':' || (p[2] != L'\\' && p[2] != L'/')) return false;
    const wchar_t root[4] = {p[0], L':', L'\\', 0};
    const UINT type = GetDriveTypeW(root);
    if (type == DRIVE_REMOTE || type == DRIVE_UNKNOWN || type == DRIVE_NO_ROOT_DIR) return false;
    const DWORD a = GetFileAttributesW(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

// Total Commander shows "d:\dir\*.*": drop a trailing wildcard component.
std::wstring StripMask(std::wstring s) {
    const size_t slash = s.find_last_of(L'\\');
    if (slash != std::wstring::npos && s.find_first_of(L"*?", slash) != std::wstring::npos) {
        s.erase(slash + 1);
        if (s.size() > 3) s.pop_back();  // keep the backslash only for a drive root ("c:\")
    }
    return s;
}

struct PathPanel { HWND hwnd; RECT rect; };
BOOL CALLBACK EnumPathPanels(HWND h, LPARAM lp) {
    if (IsWindowVisible(h) && ClassOf(h) == L"TPathPanel") {
        PathPanel p{h, {}};
        GetWindowRect(h, &p.rect);
        reinterpret_cast<std::vector<PathPanel>*>(lp)->push_back(p);
    }
    return TRUE;
}

}  // namespace

void FileManagers::Stop() {
    if (tcHook_) { UnhookWinEvent(tcHook_); tcHook_ = nullptr; }
    tcHookPid_ = 0;
    if (self_ == this) self_ = nullptr;
}

void FileManagers::NoteForeground(HWND hwnd) {
    if (!hwnd) return;
    // The user just left a file manager: this is the moment its folder is the one they chose, so keep it.
    // (Read live: the window still exists unless it was closed, in which case this yields "unknown".)
    if (active_ && hwnd != hwnd_) {
        remembered_ = ReadCurrentFolder();
        active_ = false;
        ++serial_;
    }
    const std::wstring cls = ClassOf(hwnd);
    Kind k = Kind::None;
    if (cls == L"CabinetWClass") k = Kind::Explorer;
    else if (cls == L"TTOTAL_CMD") k = Kind::TotalCommander;
    if (k == Kind::None) return;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    kind_ = k;
    hwnd_ = hwnd;
    pid_ = pid;
    active_ = true;
    if (k == Kind::TotalCommander) {
        EnsureTcHook(pid);
        CaptureTcFocus(hwnd);
    }
}

// Total Commander only reports its focused control while it is the active window, so focus changes are
// followed with a hook limited to its process. Low volume: it fires when the user switches panels.
void FileManagers::EnsureTcHook(DWORD pid) {
    if (tcHook_ && tcHookPid_ == pid) return;
    if (tcHook_) UnhookWinEvent(tcHook_);
    self_ = this;
    tcFocus_ = nullptr;
    tcHookPid_ = pid;
    tcHook_ = SetWinEventHook(EVENT_OBJECT_FOCUS, EVENT_OBJECT_FOCUS, nullptr, FocusProc, pid, 0, WINEVENT_OUTOFCONTEXT);
}

void CALLBACK FileManagers::FocusProc(HWINEVENTHOOK, DWORD, HWND hwnd, LONG idObject, LONG, DWORD, DWORD) {
    FileManagers* f = self_;
    if (!f || !hwnd || (idObject != OBJID_CLIENT && idObject != OBJID_WINDOW)) return;
    if (ClassOf(hwnd) == L"TMyListBox") f->tcFocus_ = hwnd;
}

void FileManagers::CaptureTcFocus(HWND tc) {
    GUITHREADINFO gti{sizeof(gti)};
    if (GetGUIThreadInfo(GetWindowThreadProcessId(tc, nullptr), &gti) && gti.hwndFocus &&
        ClassOf(gti.hwndFocus) == L"TMyListBox")
        tcFocus_ = gti.hwndFocus;
}

std::wstring FileManagers::ReadCurrentFolder() {
    if (kind_ == Kind::None || !hwnd_ || !IsWindow(hwnd_)) return L"";
    const std::wstring p = kind_ == Kind::Explorer ? ExplorerFolder(hwnd_) : TotalCommanderFolder(hwnd_);
    return IsLocalDirectory(p) ? p : L"";
}

// Documented route: the Shell's window list. The location URL of the matching window is a file: URL for
// real folders and a ::{GUID} for "This PC" and similar, which PathCreateFromUrl rejects.
std::wstring FileManagers::ExplorerFolder(HWND top) {
    std::wstring result;
    IShellWindows* sw = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellWindows, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&sw))) || !sw) return result;
    long n = 0;
    sw->get_Count(&n);
    for (long i = 0; i < n && result.empty(); ++i) {
        VARIANT v;
        VariantInit(&v);
        v.vt = VT_I4;
        v.lVal = i;
        IDispatch* d = nullptr;
        if (FAILED(sw->Item(v, &d)) || !d) continue;
        IWebBrowser2* wb = nullptr;
        d->QueryInterface(IID_PPV_ARGS(&wb));
        d->Release();
        if (!wb) continue;
        SHANDLE_PTR h = 0;
        if (SUCCEEDED(wb->get_HWND(&h)) && reinterpret_cast<HWND>(static_cast<INT_PTR>(h)) == top) {
            BSTR url = nullptr;
            if (SUCCEEDED(wb->get_LocationURL(&url)) && url) {
                wchar_t path[MAX_PATH]{};
                DWORD len = MAX_PATH;
                if (SUCCEEDED(PathCreateFromUrlW(url, path, &len, 0))) result = path;
                SysFreeString(url);
            }
        }
        wb->Release();
    }
    sw->Release();
    return result;
}

// The active panel is the one whose file list had focus last. Its path bar is the TPathPanel with the
// largest horizontal overlap with that list (ties, e.g. panels stacked vertically: nearest top edge).
std::wstring FileManagers::TotalCommanderFolder(HWND tc) {
    if (!tcFocus_ || !IsWindow(tcFocus_) || GetAncestor(tcFocus_, GA_ROOT) != tc) CaptureTcFocus(tc);
    if (!tcFocus_ || !IsWindow(tcFocus_) || GetAncestor(tcFocus_, GA_ROOT) != tc) return L"";  // unknown: do not guess

    std::vector<PathPanel> panels;
    EnumChildWindows(tc, EnumPathPanels, reinterpret_cast<LPARAM>(&panels));
    if (panels.empty()) return L"";

    RECT list{};
    GetWindowRect(tcFocus_, &list);
    const PathPanel* best = nullptr;
    long bestScore = LONG_MIN;
    for (const auto& p : panels) {
        const long overlap = std::max(0L, std::min(list.right, p.rect.right) - std::max(list.left, p.rect.left));
        const long score = overlap * 10000 - std::labs(list.top - p.rect.top);
        if (score > bestScore) { bestScore = score; best = &p; }
    }
    if (!best) return L"";
    return StripMask(TextOf(best->hwnd));
}
