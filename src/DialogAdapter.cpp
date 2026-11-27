#include "DialogAdapter.h"
#include <algorithm>
#include <cwctype>

namespace {

// Observed filename-edit control ids (Windows 10 22H2): 1148 = Open, 1001 = Save As, 1152 = Select Folder picker.
// Names and titles are localized and are never used for matching.
constexpr int kIdOk = 1;
constexpr int kIdEditOpen = 1148;
constexpr int kIdEditSave = 1001;
constexpr int kIdEditFolderPicker = 1152;
constexpr UINT kSendTimeoutMs = 1000;

std::wstring ClassOf(HWND h) {
    wchar_t c[128]{};
    GetClassNameW(h, c, 128);
    return c;
}

std::wstring TextOf(HWND h) {
    wchar_t buf[2048]{};
    DWORD_PTR len = 0;
    if (!SendMessageTimeoutW(h, WM_GETTEXT, 2048, reinterpret_cast<LPARAM>(buf), SMTO_ABORTIFHUNG, 500, &len))
        return L"";
    return buf;
}

struct ClassCtx { const wchar_t* cls; HWND found; };
BOOL CALLBACK EnumByClass(HWND h, LPARAM lp) {
    auto* c = reinterpret_cast<ClassCtx*>(lp);
    if (ClassOf(h) == c->cls) { c->found = h; return FALSE; }
    return TRUE;
}
HWND FindDescendant(HWND root, const wchar_t* cls) {
    ClassCtx c{cls, nullptr};
    EnumChildWindows(root, EnumByClass, reinterpret_cast<LPARAM>(&c));
    return c.found;
}

struct EditCtx { HWND found; int id; };
BOOL CALLBACK EnumEdit(HWND h, LPARAM lp) {
    auto* c = reinterpret_cast<EditCtx*>(lp);
    const int id = GetDlgCtrlID(h);
    if (ClassOf(h) == L"Edit" && (id == kIdEditOpen || id == kIdEditSave || id == kIdEditFolderPicker)) {
        c->found = h;
        c->id = id;
        return FALSE;
    }
    return TRUE;
}
HWND FindFileNameEdit(HWND dlg, int* id) {
    EditCtx c{nullptr, 0};
    EnumChildWindows(dlg, EnumEdit, reinterpret_cast<LPARAM>(&c));
    if (id) *id = c.id;
    return c.found;
}

// Replaces the whole text of the filename box the way typing over a selection does (select all, then replace).
// Plain WM_SETTEXT is NOT equivalent: in a Save As dialog that already holds a name (Notepad pre-fills one),
// the dialog then ignores the new text on OK and tries to save the old name instead. Verified on legacy and
// Common Item Save As dialogs: WM_SETTEXT failed 6/6, select-all + replace worked 16/16.
bool ReplaceEditText(HWND edit, const std::wstring& text) {
    DWORD_PTR r = 0;
    return SendMessageTimeoutW(edit, EM_SETSEL, 0, -1, SMTO_ABORTIFHUNG, kSendTimeoutMs, &r) &&
           SendMessageTimeoutW(edit, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(text.c_str()), SMTO_ABORTIFHUNG,
                               kSendTimeoutMs, &r);
}

std::wstring Normalize(std::wstring s) {
    while (s.size() > 3 && (s.back() == L'\\' || s.back() == L'/')) s.pop_back();
    std::transform(s.begin(), s.end(), s.begin(), [](wchar_t ch) { return static_cast<wchar_t>(std::towlower(ch)); });
    return s;
}

}  // namespace

namespace DialogAdapter {

bool IsCandidateClass(HWND h) { return h && ClassOf(h) == L"#32770"; }

bool IsSupported(HWND dlg) {
    if (!dlg || !IsWindow(dlg) || !IsWindowVisible(dlg) || !IsCandidateClass(dlg)) return false;
    DWORD pid = 0;
    GetWindowThreadProcessId(dlg, &pid);
    if (pid == GetCurrentProcessId()) return false;
    if (!GetDlgItem(dlg, kIdOk)) return false;
    return FindFileNameEdit(dlg, nullptr) != nullptr;  // Open, Save As and the Select Folder picker
}

// The breadcrumb toolbar under "Address Band Root" has the text "<localized label>: <path>".
std::wstring CurrentFolder(HWND dlg) {
    HWND band = FindDescendant(dlg, L"Address Band Root");
    HWND parent = band ? FindDescendant(band, L"Breadcrumb Parent") : nullptr;
    HWND toolbar = parent ? FindDescendant(parent, L"ToolbarWindow32") : nullptr;
    if (!toolbar) return L"";
    const std::wstring t = TextOf(toolbar);
    const size_t p = t.find(L": ");
    return p == std::wstring::npos ? t : t.substr(p + 2);
}

bool Navigate(HWND dlg, const std::wstring& folder) {
    HWND edit = FindFileNameEdit(dlg, nullptr);
    HWND ok = GetDlgItem(dlg, kIdOk);
    if (!edit || !ok) return false;
    if (!ReplaceEditText(edit, folder)) return false;
    // Same as pressing the default button: a folder path in the filename box navigates. This holds in the Select Folder
    // picker too (measured: the dialog moves to the folder and stays open; pressing "Select Folder" afterwards returns
    // exactly that folder). An earlier version refused the picker on the untested assumption that OK would close it.
    DWORD_PTR r = 0;
    return SendMessageTimeoutW(dlg, WM_COMMAND, MAKEWPARAM(kIdOk, BN_CLICKED), reinterpret_cast<LPARAM>(ok),
                               SMTO_ABORTIFHUNG, kSendTimeoutMs, &r) != 0;
}

std::wstring GetFileName(HWND dlg) {
    HWND edit = FindFileNameEdit(dlg, nullptr);
    return edit ? TextOf(edit) : L"";
}

bool SetFileName(HWND dlg, const std::wstring& text) {
    HWND edit = FindFileNameEdit(dlg, nullptr);
    return edit && ReplaceEditText(edit, text);
}

bool SamePath(const std::wstring& a, const std::wstring& b) { return Normalize(a) == Normalize(b); }

}  // namespace DialogAdapter
