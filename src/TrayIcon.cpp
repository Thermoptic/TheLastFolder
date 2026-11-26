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

HICON LoadIconFromFile(const wchar_t* path, int size) {
    return static_cast<HICON>(LoadImageW(nullptr, path, IMAGE_ICON, size, size, LR_LOADFROMFILE | LR_DEFAULTCOLOR));
}

TrayIcon::~TrayIcon() {
    Remove();
    if (icon_ && ownsIcon_) DestroyIcon(icon_);
}

bool TrayIcon::Add(HWND owner, UINT callbackMsg, const wchar_t* tooltip) {
    hwnd_ = owner;
    callbackMsg_ = callbackMsg;
    wcsncpy_s(tip_, tooltip, _TRUNCATE);
    if (!icon_) SetDefaultIcon();  // nothing chosen before Add(): start with the built-in icon
    return AddInternal();
}

void TrayIcon::ReplaceIcon(HICON icon, bool owned) {
    HICON old = icon_;
    const bool oldOwned = ownsIcon_;
    icon_ = icon;
    ownsIcon_ = owned;
    if (added_) {
        NOTIFYICONDATAW nid{sizeof(nid)};
        nid.hWnd = hwnd_;
        nid.uID = kIconId;
        nid.uFlags = NIF_ICON;
        nid.hIcon = icon_;
        Shell_NotifyIconW(NIM_MODIFY, &nid);
    }
    if (old && oldOwned) DestroyIcon(old);  // only after the tray stopped using it
}

bool TrayIcon::SetIconFromFile(const wchar_t* path) {
    HICON icon = LoadIconFromFile(path, GetSystemMetrics(SM_CXSMICON));
    if (!icon) return false;
    ReplaceIcon(icon, true);
    return true;
}

void TrayIcon::SetDefaultIcon() {
    HICON icon = LoadAppIcon(GetSystemMetrics(SM_CXSMICON));
    if (icon) ReplaceIcon(icon, true);
    else ReplaceIcon(LoadIconW(nullptr, IDI_APPLICATION), false);  // shared system icon; never destroyed by us
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
    for (size_t i = 0; i < count; ++i) {
        if (items[i].id == 0) AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        else AppendMenuW(menu, MF_STRING | (items[i].checked ? MF_CHECKED : 0), items[i].id, items[i].text);
    }
    // Required so the menu closes when the user clicks elsewhere.
    SetForegroundWindow(hwnd_);
    TrackPopupMenuEx(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, hwnd_, nullptr);
    PostMessageW(hwnd_, WM_NULL, 0, 0);
    DestroyMenu(menu);
}
