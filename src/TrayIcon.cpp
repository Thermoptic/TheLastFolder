#include "TrayIcon.h"
#include <shellapi.h>
#include "resource.h"

namespace {

constexpr UINT kIconId = 1;

// The tray icon is the app's own icon resource (assets/icon.ico, embedded by app.rc). The size is the system's
// small-icon size: 16x16 at 100% scaling, larger on high-DPI screens, where the nearest image in the file is used.
HICON LoadAppIcon(int size) {
    return static_cast<HICON>(
        LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON, size, size, LR_DEFAULTCOLOR));
}

}  // namespace

TrayIcon::~TrayIcon() {
    Remove();
    if (icon_ && ownsIcon_) DestroyIcon(icon_);
}

bool TrayIcon::Add(HWND owner, UINT callbackMsg, const wchar_t* tooltip) {
    hwnd_ = owner;
    callbackMsg_ = callbackMsg;
    wcsncpy_s(tip_, tooltip, _TRUNCATE);
    if (!icon_) {
        icon_ = LoadAppIcon(GetSystemMetrics(SM_CXSMICON));
        ownsIcon_ = icon_ != nullptr;
        if (!icon_) icon_ = LoadIconW(nullptr, IDI_APPLICATION);  // shared system icon; never destroyed by us
    }
    return AddInternal();
}

bool TrayIcon::AddInternal() {
    NOTIFYICONDATAW nid{sizeof(nid)};
    nid.hWnd = hwnd_;
    nid.uID = kIconId;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    nid.uCallbackMessage = callbackMsg_;
    nid.hIcon = icon_;
    wcsncpy_s(nid.szTip, tip_, _TRUNCATE);
    if (!Shell_NotifyIconW(NIM_ADD, &nid)) return false;
    nid.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &nid);
    added_ = true;
    return true;
}

void TrayIcon::Readd() {
    if (hwnd_) AddInternal();  // fails harmlessly if the icon already exists
}

void TrayIcon::Remove() {
    if (!added_) return;
    NOTIFYICONDATAW nid{sizeof(nid)};
    nid.hWnd = hwnd_;
    nid.uID = kIconId;
    Shell_NotifyIconW(NIM_DELETE, &nid);
    added_ = false;
}

void TrayIcon::ShowMenu(POINT pt, const MenuItem* items, size_t count) {
    HMENU menu = CreatePopupMenu();
    if (!menu) return;
    for (size_t i = 0; i < count; ++i) AppendMenuW(menu, MF_STRING, items[i].id, items[i].text);
    // Required so the menu closes when the user clicks elsewhere.
    SetForegroundWindow(hwnd_);
    TrackPopupMenuEx(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, hwnd_, nullptr);
    PostMessageW(hwnd_, WM_NULL, 0, 0);
    DestroyMenu(menu);
}
