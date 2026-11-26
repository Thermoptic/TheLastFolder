#include "SetIconDialog.h"
#include <commdlg.h>
#include <string>
#include "Settings.h"
#include "TrayIcon.h"  // LoadIconFromFile, used to check that a chosen file really is an icon
#include "resource.h"

// Visual styles for the dialog's controls need common controls v6.
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace {

struct State {
    std::wstring path;  // the file chosen with Browse (or remembered from last time); independent of the check box
};

void Note(HWND dlg, const wchar_t* text) { MessageBoxW(dlg, text, L"Set Icon", MB_OK | MB_ICONINFORMATION); }

// Browse and the file name are only live while the box is ticked; unticked, the remembered file stays visible but dimmed.
void SyncControls(HWND dlg, const State& s) {
    const bool on = IsDlgButtonChecked(dlg, IDC_CUSTOM) == BST_CHECKED;
    EnableWindow(GetDlgItem(dlg, IDC_BROWSE), on);
    EnableWindow(GetDlgItem(dlg, IDC_PATH), on);
    SetDlgItemTextW(dlg, IDC_PATH, s.path.empty() ? L"(no file chosen)" : s.path.c_str());
}

bool Browse(HWND dlg, State& s) {
    wchar_t file[MAX_PATH * 2]{};
    wcsncpy_s(file, s.path.c_str(), _TRUNCATE);  // opens in the remembered file's folder
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = dlg;
    ofn.lpstrFile = file;
    ofn.nMaxFile = ARRAYSIZE(file);
    ofn.lpstrFilter = L"Icon files (*.ico)\0*.ico\0";
    ofn.lpstrTitle = L"Choose an icon";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_HIDEREADONLY;
    if (!GetOpenFileNameW(&ofn)) return false;
    s.path = file;
    return true;
}

// Apply: validate, save, close. Stays open (with a short explanation) if the choice cannot work.
void Apply(HWND dlg, State& s) {
    Settings::CustomIcon value;
    value.enabled = IsDlgButtonChecked(dlg, IDC_CUSTOM) == BST_CHECKED;
    value.path = s.path;  // kept even when unticked

    if (value.enabled) {
        if (value.path.empty()) { Note(dlg, L"Choose an icon file first: click Browse."); return; }
        HICON test = LoadIconFromFile(value.path.c_str(), 16);
        if (!test) { Note(dlg, L"That file could not be loaded as an icon. Choose an .ico file."); return; }
        DestroyIcon(test);
    }
    if (!Settings::SaveCustomIcon(value)) { Note(dlg, L"The setting could not be saved."); return; }
    EndDialog(dlg, IDOK);
}

INT_PTR CALLBACK DlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_INITDIALOG: {
            auto* s = reinterpret_cast<State*>(lp);
            SetWindowLongPtrW(dlg, DWLP_USER, reinterpret_cast<LONG_PTR>(s));
            const Settings::CustomIcon saved = Settings::LoadCustomIcon();
            s->path = saved.path;
            CheckDlgButton(dlg, IDC_CUSTOM, saved.enabled ? BST_CHECKED : BST_UNCHECKED);
            SyncControls(dlg, *s);
            SetForegroundWindow(dlg);  // the owner window is hidden, so make sure the dialog is in front
            return TRUE;
        }
        case WM_COMMAND: {
            auto* s = reinterpret_cast<State*>(GetWindowLongPtrW(dlg, DWLP_USER));
            if (!s) return FALSE;
            switch (LOWORD(wp)) {
                case IDC_CUSTOM:
                    if (HIWORD(wp) == BN_CLICKED) SyncControls(dlg, *s);
                    return TRUE;
                case IDC_BROWSE:
                    if (Browse(dlg, *s)) SyncControls(dlg, *s);
                    return TRUE;
                case IDOK:
                    Apply(dlg, *s);
                    return TRUE;
                case IDCANCEL:
                    EndDialog(dlg, IDCANCEL);
                    return TRUE;
            }
            return FALSE;
        }
    }
    return FALSE;
}

}  // namespace

bool ShowSetIconDialog(HINSTANCE inst, HWND owner) {
    State state;
    return DialogBoxParamW(inst, MAKEINTRESOURCEW(IDD_SETICON), owner, DlgProc, reinterpret_cast<LPARAM>(&state)) == IDOK;
}
