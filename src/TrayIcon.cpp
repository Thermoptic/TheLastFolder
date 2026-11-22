#include "TrayIcon.h"
#include <shellapi.h>
#include <vector>
#include <cstdint>

namespace {

constexpr UINT kIconId = 1;

// Draws a small folder (back, tab, front, outline) into a 32-bit ARGB bitmap. No resource file needed.
HICON CreateFolderIcon(int size) {
    std::vector<uint32_t> px(size_t(size) * size, 0);  // transparent
    auto rect = [&](float x0, float y0, float x1, float y1, uint32_t argb) {
        const float k = size / 16.0f;
        for (int y = int(y0 * k + 0.5f); y < int(y1 * k + 0.5f) && y < size; ++y)
            for (int x = int(x0 * k + 0.5f); x < int(x1 * k + 0.5f) && x < size; ++x)
                px[size_t(y) * size + x] = argb;
    };
    constexpr uint32_t kOutline = 0xFF8A6A12, kBack = 0xFFD9A520, kFront = 0xFFF7D154;
    rect(1, 2, 7, 5, kOutline);     // tab
    rect(2, 3, 6, 5, kBack);
    rect(1, 4, 15, 14, kOutline);   // body outline
    rect(2, 5, 14, 13, kBack);      // back panel
    rect(2, 7, 14, 13, kFront);     // front panel

    BITMAPV5HEADER bi{};
    bi.bV5Size = sizeof(bi);
    bi.bV5Width = size;
    bi.bV5Height = -size;  // top-down
    bi.bV5Planes = 1;
    bi.bV5BitCount = 32;
    bi.bV5Compression = BI_BITFIELDS;
    bi.bV5RedMask = 0x00FF0000; bi.bV5GreenMask = 0x0000FF00; bi.bV5BlueMask = 0x000000FF; bi.bV5AlphaMask = 0xFF000000;

    void* bits = nullptr;
    HDC dc = GetDC(nullptr);
    HBITMAP color = CreateDIBSection(dc, reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS, &bits, nullptr, 0);
    ReleaseDC(nullptr, dc);
    HBITMAP mask = CreateBitmap(size, size, 1, 1, nullptr);
    HICON icon = nullptr;
    if (color && mask && bits) {
        memcpy(bits, px.data(), px.size() * sizeof(uint32_t));
        ICONINFO ii{TRUE, 0, 0, mask, color};
        icon = CreateIconIndirect(&ii);
    }
    if (color) DeleteObject(color);
    if (mask) DeleteObject(mask);
    return icon;
}

}  // namespace

TrayIcon::~TrayIcon() {
    Remove();
    if (icon_) DestroyIcon(icon_);
}

bool TrayIcon::Add(HWND owner, UINT callbackMsg, const wchar_t* tooltip) {
    hwnd_ = owner;
    callbackMsg_ = callbackMsg;
    wcsncpy_s(tip_, tooltip, _TRUNCATE);
    if (!icon_) {
        icon_ = CreateFolderIcon(GetSystemMetrics(SM_CXSMICON));
        if (!icon_) icon_ = LoadIconW(nullptr, IDI_APPLICATION);  // shared; never destroyed by us
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
