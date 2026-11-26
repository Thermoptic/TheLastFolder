// TrayIcon: notification-area icon and its context menu. Owns the HICON.
#pragma once
#include <windows.h>

struct MenuItem {
    UINT id;                  // command id delivered as WM_COMMAND; 0 makes a separator line (text is ignored)
    const wchar_t* text;
    bool checked = false;     // draws a check mark in front of the item
};

// Loads an icon from an .ico file at `size` x `size` (the closest image in the file is used). Caller destroys it.
// Returns null if the file is missing or is not a usable icon.
HICON LoadIconFromFile(const wchar_t* path, int size);

class TrayIcon {
public:
    TrayIcon() = default;
    ~TrayIcon();
    TrayIcon(const TrayIcon&) = delete;
    TrayIcon& operator=(const TrayIcon&) = delete;

    // Adds the icon. `callbackMsg` is delivered to `owner` (NOTIFYICON_VERSION_4 semantics).
    bool Add(HWND owner, UINT callbackMsg, const wchar_t* tooltip);
    // Re-adds the icon after Explorer restarts (TaskbarCreated). Harmless if the icon still exists.
    void Readd();
    void Remove();

    // Switches the icon shown in the tray. Safe before Add() (takes effect when the icon is added).
    // SetIconFromFile returns false and leaves the current icon alone if the file cannot be loaded.
    bool SetIconFromFile(const wchar_t* path);
    void SetDefaultIcon();  // the built-in icon (assets/icon.ico, embedded in the exe)

    // Shows the context menu at screen position `pt`; chosen item arrives as WM_COMMAND on the owner.
    void ShowMenu(POINT pt, const MenuItem* items, size_t count);

private:
    bool AddInternal();
    void ReplaceIcon(HICON icon, bool owned);  // swaps icon_, updates the tray if it is shown, frees the old one

    HWND hwnd_ = nullptr;
    UINT callbackMsg_ = 0;
    wchar_t tip_[128]{};
    HICON icon_ = nullptr;
    bool ownsIcon_ = false;  // true if icon_ came from our resource (must be destroyed), false for a shared system icon
    bool added_ = false;
};
