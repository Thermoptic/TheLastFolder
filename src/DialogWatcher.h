// DialogWatcher: event-driven (WinEvent, out-of-context) tracking of one supported file dialog.
// No polling. A global hook sees only foreground changes; the per-dialog hook is installed for the
// dialog's own thread while one is tracked (so mouse/caret location events never wake us).
#pragma once
#include <windows.h>
#include <functional>

class DialogWatcher {
public:
    enum class Event { Active, Lost, Moved, Other };
    // Active: a supported dialog became the tracked one. Lost: it closed, hid, or focus left it
    // (hwnd is the old dialog). Moved: it moved or resized. Other: some non-dialog window became the
    // foreground window (hwnd); lets other modules follow focus without a second hook.
    using Callback = std::function<void(Event, HWND)>;

    DialogWatcher() = default;
    ~DialogWatcher() { Stop(); }
    DialogWatcher(const DialogWatcher&) = delete;
    DialogWatcher& operator=(const DialogWatcher&) = delete;

    bool Start(Callback cb);
    void Stop();
    HWND Current() const { return current_; }

private:
    static void CALLBACK ForegroundProc(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD);
    static void CALLBACK DialogProc(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD);
    static void CALLBACK RetryTimer(HWND, UINT, UINT_PTR, DWORD);

    void OnForeground(HWND hwnd);
    void Track(HWND dlg);
    void Drop();
    void StartRetry(HWND hwnd);
    void CancelRetry();

    static DialogWatcher* self_;
    Callback cb_;
    HWINEVENTHOOK fgHook_ = nullptr;
    HWINEVENTHOOK dlgHook_ = nullptr;
    HWND current_ = nullptr;
    HWND pending_ = nullptr;   // dialog-class window whose controls may not exist yet
    UINT_PTR retryTimer_ = 0;
    int retries_ = 0;
};
